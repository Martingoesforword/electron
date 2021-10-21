// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/app/electron_main.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(OS_POSIX)
#include <sys/stat.h>
#endif

#if defined(OS_WIN)
#include <windows.h>  // 必须先包含windows.h。

#include <atlbase.h>  // 确保像`_AtlWinModule`这样的ATL静态被初始化(这是静态调试版本中的一个问题)。
#include <shellapi.h>
#include <shellscalingapi.h>
#include <tchar.h>

#include "base/environment.h"
#include "base/process/launch.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "components/browser_watcher/exit_code_watcher_win.h"
#include "components/crash/core/app/crash_switches.h"
#include "components/crash/core/app/run_as_crashpad_handler_win.h"
#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"
#include "shell/app/command_line_args.h"
#include "shell/app/electron_main_delegate.h"
#include "third_party/crashpad/crashpad/util/win/initial_client_data.h"

#elif defined(OS_LINUX)  // 已定义(OS_WIN)。
#include <unistd.h>
#include <cstdio>
#include "base/base_switches.h"
#include "base/command_line.h"
#include "content/public/app/content_main.h"
#include "shell/app/electron_main_delegate.h"  // NOLINT。
#else                                          // 已定义(OS_Linux)。
#include <mach-o/dyld.h>
#include <unistd.h>
#include <cstdio>
#include "shell/app/electron_library_main.h"
#endif  // 已定义(OS_MAC)。

#include "base/at_exit.h"
#include "base/i18n/icu_util.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "shell/app/node_main.h"
#include "shell/common/electron_command_line.h"
#include "shell/common/electron_constants.h"

#if defined(HELPER_EXECUTABLE) && !defined(MAS_BUILD)
#include "sandbox/mac/seatbelt_exec.h"  // 点名检查。
#endif

namespace {

#if defined(OS_WIN)
// 在这里重新定义，这样我们就不必引入对//内容的依赖。
// 发信人//电子邮件：ELECTORE_APP。
const char kUserDataDir[] = "user-data-dir";
const char kProcessType[] = "type";
#endif

ALLOW_UNUSED_TYPE bool IsEnvSet(const char* name) {
#if defined(OS_WIN)
  size_t required_size;
  getenv_s(&required_size, nullptr, 0, name);
  return required_size != 0;
#else
  char* indicator = getenv(name);
  return indicator && indicator[0] != '\0';
#endif
}

#if defined(OS_POSIX)
void FixStdioStreams() {
  // Libuv可能会将stdin/stdout/stderr标记为执行结束，这会干扰。
  // 铬的子过程产卵。作为一种解决方法，我们检测到这些。
  // 流在启动时关闭，如果需要，以/dev/null的身份重新打开它们。
  // 否则，不相关的文件描述符将被指定为stdout/stderr。
  // 这在试图写入它们时可能导致各种错误。
  // 
  // 有关详细信息，请参阅https://github.com/libuv/libuv/issues/2062。
  struct stat st;
  if (fstat(STDIN_FILENO, &st) < 0 && errno == EBADF)
    ignore_result(freopen("/dev/null", "r", stdin));
  if (fstat(STDOUT_FILENO, &st) < 0 && errno == EBADF)
    ignore_result(freopen("/dev/null", "w", stdout));
  if (fstat(STDERR_FILENO, &st) < 0 && errno == EBADF)
    ignore_result(freopen("/dev/null", "w", stderr));
}
#endif

}  // 命名空间。

#if defined(OS_WIN)

namespace crash_reporter {
extern const char kCrashpadProcess[];
}

// 在32位构建中，主线程从默认(小)堆栈大小开始。
// 此处和下面的ARCH_CPU_32_BITS块支持将Main。
// 将螺纹连接到堆叠尺寸较大的纤维上。
#if defined(ARCH_CPU_32_BITS)
// 将控制转移到大堆叠光纤及以后所需的信息。
// 将主例程的退出代码传递回小堆栈纤程。
// 终止。
struct FiberState {
  HINSTANCE instance;
  LPVOID original_fiber;
  int fiber_result;
};

// PFIBER_START_ROUTINE函数在调用。
// Main例程，存储其返回值，并将控制权返回给小堆栈。
// 纤维。|params|必须是指向FiberState结构的指针。
void WINAPI FiberBinder(void* params) {
  auto* fiber_state = static_cast<FiberState*>(params);
  // 从纤程调用wWinMain例程。重用入口点可以最小化入口点。
  // 检查崩溃报告中的调用堆栈时出现混乱-查看wWinMain打开。
  // 堆栈是一个方便的提示，表明这是进程的主线程。
  fiber_state->fiber_result =
      wWinMain(fiber_state->instance, nullptr, nullptr, 0);
  // 切换回主线程以退出。
  ::SwitchToFiber(fiber_state->original_fiber);
}
#endif  // 已定义(ARCH_CPU_32_BITS)。

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmd, int) {
#if defined(ARCH_CPU_32_BITS)
  enum class FiberStatus { kConvertFailed, kCreateFiberFailed, kSuccess };
  FiberStatus fiber_status = FiberStatus::kSuccess;
  // 如果光纤转换失败，则返回GetLastError结果。
  DWORD fiber_error = ERROR_SUCCESS;
  if (!::IsThreadAFiber()) {
    // 将主线程的堆栈大小设置为4MiB，以便它具有大致相同的。
    // 有效大小为64位版本的8MiB堆栈。
    constexpr size_t kStackSize = 4 * 1024 * 1024;  // 4 MiB。
    // 出口处的光纤泄漏。
    LPVOID original_fiber =
        ::ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
    if (original_fiber) {
      FiberState fiber_state = {instance, original_fiber};
      // 创建一根堆叠更大的光纤，然后切换到它。把纤维泄漏到。
      // 出口。
      LPVOID big_stack_fiber = ::CreateFiberEx(
          0, kStackSize, FIBER_FLAG_FLOAT_SWITCH, FiberBinder, &fiber_state);
      if (big_stack_fiber) {
        ::SwitchToFiber(big_stack_fiber);
        // 必须清理光纤，以避免模糊的TLS相关关闭。
        // 坠毁。
        ::DeleteFiber(big_stack_fiber);
        ::ConvertFiberToThread();
        // Chrome在FiberMain上运行完毕后，控制返回此处。
        return fiber_state.fiber_result;
      }
      fiber_status = FiberStatus::kCreateFiberFailed;
    } else {
      fiber_status = FiberStatus::kConvertFailed;
    }
    // 如果我们到达这里，那么创建和切换到光纤就失败了。这。
    // 可能意味着我们内存不足，很快就会崩溃。试着报告。
    // 一旦崩溃报告被初始化，就会出现此错误。
    fiber_error = ::GetLastError();
    base::debug::Alias(&fiber_error);
  }
  // 如果我们已经是光纤，那么继续正常执行。
#endif  // 已定义(ARCH_CPU_32_BITS)。

  struct Arguments {
    int argc = 0;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    ~Arguments() { LocalFree(argv); }
  } arguments;

  if (!arguments.argv)
    return -1;

#ifdef _DEBUG
  // 在CI测试运行中不显示Assert对话框。
  static const char kCI[] = "CI";
  if (IsEnvSet(kCI)) {
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

    _set_error_mode(_OUT_TO_STDERR);
  }
#endif

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  bool run_as_node =
      electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode);
#else
  bool run_as_node = false;
#endif

  // 确保将输出打印到控制台。
  if (run_as_node || !IsEnvSet("ELECTRON_NO_ATTACH_CONSOLE"))
    base::RouteStdioToConsole(false);

  std::vector<char*> argv(arguments.argc);
  std::transform(arguments.argv, arguments.argv + arguments.argc, argv.begin(),
                 [](auto& a) { return _strdup(base::WideToUTF8(a).c_str()); });
#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (electron::fuses::IsRunAsNodeEnabled() && run_as_node) {
    base::AtExitManager atexit_manager;
    base::i18n::InitializeICU();
    auto ret = electron::NodeMain(argv.size(), argv.data());
    std::for_each(argv.begin(), argv.end(), free);
    return ret;
  }
#endif

  base::CommandLine::Init(argv.size(), argv.data());
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  const std::string process_type =
      command_line->GetSwitchValueASCII(kProcessType);

  if (process_type == crash_reporter::switches::kCrashpadHandler) {
    // 检查我们是否应该监视此进程的退出代码。
    std::unique_ptr<browser_watcher::ExitCodeWatcher> exit_code_watcher;

    // 从命令行检索客户端进程。
    crashpad::InitialClientData initial_client_data;
    if (initial_client_data.InitializeFromString(
            command_line->GetSwitchValueASCII("initial-client-data"))) {
      // 设置退出代码观察器以监视父进程。
      HANDLE duplicate_handle = INVALID_HANDLE_VALUE;
      if (DuplicateHandle(
              ::GetCurrentProcess(), initial_client_data.client_process(),
              ::GetCurrentProcess(), &duplicate_handle,
              PROCESS_QUERY_INFORMATION, FALSE, DUPLICATE_SAME_ACCESS)) {
        base::Process parent_process(duplicate_handle);
        exit_code_watcher =
            std::make_unique<browser_watcher::ExitCodeWatcher>();
        if (exit_code_watcher->Initialize(std::move(parent_process))) {
          exit_code_watcher->StartWatching();
        }
      }
    }

    // 必须始终向处理程序进程传递。
    // 命令行。
    DCHECK(command_line->HasSwitch(kUserDataDir));

    base::FilePath user_data_dir =
        command_line->GetSwitchValuePath(kUserDataDir);
    int crashpad_status = crash_reporter::RunAsCrashpadHandler(
        *command_line, user_data_dir, kProcessType, kUserDataDir);
    if (crashpad_status != 0 && exit_code_watcher) {
      // CrashPad无法初始化，请显式停止退出代码监视器。
      // 因此，CrashPad-Handler进程可以退出并返回错误。
      exit_code_watcher->StopWatching();
    }
    return crashpad_status;
  }

#if defined(ARCH_CPU_32_BITS)
  // 如果转换为光纤失败，则故意崩溃。
  CHECK_EQ(fiber_status, FiberStatus::kSuccess);
#endif  // 已定义(ARCH_CPU_32_BITS)。

  if (!electron::CheckCommandLineArguments(arguments.argc, arguments.argv))
    return -1;

  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  content::InitializeSandboxInfo(&sandbox_info);
  electron::ElectronMainDelegate delegate;

  content::ContentMainParams params(&delegate);
  params.instance = instance;
  params.sandbox_info = &sandbox_info;
  electron::ElectronCommandLine::Init(arguments.argc, arguments.argv);
  return content::ContentMain(params);
}

#elif defined(OS_LINUX)  // 已定义(OS_WIN)。

int main(int argc, char* argv[]) {
  FixStdioStreams();

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode)) {
    base::i18n::InitializeICU();
    base::AtExitManager atexit_manager;
    return electron::NodeMain(argc, argv);
  }
#endif

  electron::ElectronMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  electron::ElectronCommandLine::Init(argc, argv);
  params.argc = argc;
  params.argv = const_cast<const char**>(argv);
  base::CommandLine::Init(params.argc, params.argv);
  // TODO(完全迁移Chrome Linux时删除https://crbug.com/1176772)：
  // 去Crashpad。
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      ::switches::kEnableCrashpad);
  return content::ContentMain(params);
}

#else  // 已定义(OS_Linux)。

int main(int argc, char* argv[]) {
  FixStdioStreams();

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode)) {
    return ElectronInitializeICUandStartNode(argc, argv);
  }
#endif

#if defined(HELPER_EXECUTABLE) && !defined(MAS_BUILD)
  uint32_t exec_path_size = 0;
  int rv = _NSGetExecutablePath(NULL, &exec_path_size);
  if (rv != -1) {
    fprintf(stderr, "_NSGetExecutablePath: get length failed\n");
    abort();
  }

  auto exec_path = std::make_unique<char[]>(exec_path_size);
  rv = _NSGetExecutablePath(exec_path.get(), &exec_path_size);
  if (rv != 0) {
    fprintf(stderr, "_NSGetExecutablePath: get path failed\n");
    abort();
  }
  sandbox::SeatbeltExecServer::CreateFromArgumentsResult seatbelt =
      sandbox::SeatbeltExecServer::CreateFromArguments(exec_path.get(), argc,
                                                       argv);
  if (seatbelt.sandbox_required) {
    if (!seatbelt.server) {
      fprintf(stderr, "Failed to create seatbelt sandbox server.\n");
      abort();
    }
    if (!seatbelt.server->InitializeSandbox()) {
      fprintf(stderr, "Failed to initialize sandbox.\n");
      abort();
    }
  }
#endif  // 已定义(HELPER_EXECUTABLE)&&！已定义(MAS_BUILD)。

  return ElectronMain(argc, argv);
}

#endif  // 已定义(OS_MAC)
