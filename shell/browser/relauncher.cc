// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/relauncher.h"

#include <utility>

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "shell/common/electron_command_line.h"

#if defined(OS_POSIX)
#include "base/posix/eintr_wrapper.h"
#endif

namespace relauncher {

namespace internal {

#if defined(OS_POSIX)
const int kRelauncherSyncFD = STDERR_FILENO + 1;
#endif

const CharType* kRelauncherTypeArg = FILE_PATH_LITERAL("--type=relauncher");
const CharType* kRelauncherArgSeparator = FILE_PATH_LITERAL("---");

}  // 命名空间内部。

bool RelaunchApp(const StringVector& argv) {
  // 使用当前运行的应用程序的帮助器进程。自动的。
  // 更新功能会小心地保留当前运行的版本，
  // 因此，这是安全的，即使重新启动是在以下情况下进行更新的结果。
  // 已应用。事实上，它比使用更新版本的。
  // 帮助器进程，因为不能保证更新版本的。
  // 重新启动程序的实现将与正在运行的版本兼容。
  base::FilePath child_path;
  if (!base::PathService::Get(content::CHILD_PROCESS_EXE, &child_path)) {
    LOG(ERROR) << "No CHILD_PROCESS_EXE";
    return false;
  }

  StringVector relauncher_args;
  return RelaunchAppWithHelper(child_path, relauncher_args, argv);
}

bool RelaunchAppWithHelper(const base::FilePath& helper,
                           const StringVector& relauncher_args,
                           const StringVector& argv) {
  StringVector relaunch_argv;
  relaunch_argv.push_back(helper.value());
  relaunch_argv.push_back(internal::kRelauncherTypeArg);
  // Relauncher进程有自己的--type=relauncher。
  // SERVICE_MANAGER无法识别，显式设置。
  // 中避免签入失败的沙盒类型。
  // SERVICE_MANAGER：：SandboxTypeFromCommandLine。
  relaunch_argv.push_back(FILE_PATH_LITERAL("--no-sandbox"));

  relaunch_argv.insert(relaunch_argv.end(), relauncher_args.begin(),
                       relauncher_args.end());

  relaunch_argv.push_back(internal::kRelauncherArgSeparator);

  relaunch_argv.insert(relaunch_argv.end(), argv.begin(), argv.end());

#if defined(OS_POSIX)
  int pipe_fds[2];
  if (HANDLE_EINTR(pipe(pipe_fds)) != 0) {
    PLOG(ERROR) << "pipe";
    return false;
  }

  // 父进程将仅使用PIPE_READ_FD作为。
  // 管子。一旦重新启动程序进程完成，它就可以关闭写入端。
  // 付了钱。重新启动程序进程将仅使用PIPE_WRITE_FD作为。
  // 在管子的一侧写字。在该过程中，读取侧将通过以下方式关闭。
  // Base：：LaunchApp，因为它不会出现在FD_MAP中，而写入端。
  // 将通过FD_MAP重新映射到kRelauncherSyncFD。
  base::ScopedFD pipe_read_fd(pipe_fds[0]);
  base::ScopedFD pipe_write_fd(pipe_fds[1]);

  // 确保kRelauncherSyncFD为安全值。Base：：LaunchProcess将。
  // 在派生进程中保留这三个FD，因此kRelauncherSyncFD应该。
  // 而不是与他们发生冲突。
  static_assert(internal::kRelauncherSyncFD != STDIN_FILENO &&
                    internal::kRelauncherSyncFD != STDOUT_FILENO &&
                    internal::kRelauncherSyncFD != STDERR_FILENO,
                "kRelauncherSyncFD must not conflict with stdio fds");
#endif

  base::LaunchOptions options;
#if defined(OS_POSIX)
  options.fds_to_remap.push_back(
      std::make_pair(pipe_write_fd.get(), internal::kRelauncherSyncFD));
  base::Process process = base::LaunchProcess(relaunch_argv, options);
#elif defined(OS_WIN)
  base::Process process = base::LaunchProcess(
      internal::ArgvToCommandLineString(relaunch_argv), options);
#endif
  if (!process.IsValid()) {
    LOG(ERROR) << "base::LaunchProcess failed";
    return false;
  }

  // 重新启动程序进程现在正在启动，或已经启动。这个。
  // 原始父进程将继续。

#if defined(OS_WIN)
  // 与重新启动程序进程同步。
  StringType name = internal::GetWaitEventName(process.Pid());
  HANDLE wait_event = ::CreateEventW(NULL, TRUE, FALSE, name.c_str());
  if (wait_event != NULL) {
    WaitForSingleObject(wait_event, 1000);
    CloseHandle(wait_event);
  }
#elif defined(OS_POSIX)
  pipe_write_fd.reset();  // CLOSE(PIPE_FDS[1])；

  // 与重新启动程序进程同步。
  char read_char;
  int read_result = HANDLE_EINTR(read(pipe_read_fd.get(), &read_char, 1));
  if (read_result != 1) {
    if (read_result < 0) {
      PLOG(ERROR) << "read";
    } else {
      LOG(ERROR) << "read: unexpected result " << read_result;
    }
    return false;
  }

  // 由于已从重新启动器进程中成功读取了一个字节，因此它的。
  // 保证已经设置了它的kQueue，监视该进程是否退出。
  // 现在可以安全出口了。
#endif
  return true;
}

int RelauncherMain(const content::MainFunctionParams& main_parameters) {
  const StringVector& argv = electron::ElectronCommandLine::argv();

  if (argv.size() < 4 || argv[1] != internal::kRelauncherTypeArg) {
    LOG(ERROR) << "relauncher process invoked with unexpected arguments";
    return 1;
  }

  internal::RelauncherSynchronizeWithParent();

  // 确定要执行的内容、要传递的参数以及是否。
  // 在后台启动它。
  bool in_relauncher_args = false;
  StringType relaunch_executable;
  StringVector relauncher_args;
  StringVector launch_argv;
  for (size_t argv_index = 2; argv_index < argv.size(); ++argv_index) {
    const StringType& arg(argv[argv_index]);
    if (!in_relauncher_args) {
      if (arg == internal::kRelauncherArgSeparator) {
        in_relauncher_args = true;
      } else {
        relauncher_args.push_back(arg);
      }
    } else {
      launch_argv.push_back(arg);
    }
  }

  if (launch_argv.empty()) {
    LOG(ERROR) << "nothing to relaunch";
    return 1;
  }

  if (internal::LaunchProgram(relauncher_args, launch_argv) != 0) {
    LOG(ERROR) << "failed to launch program";
    return 1;
  }

  // 应用程序应该已重新启动(或正在重新启动。
  // 重新启动)。从这一点开始，应该只执行清理任务，并且。
  // 失败是可以容忍的。

  return 0;
}

}  // 命名空间重新启动器
