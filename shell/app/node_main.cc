// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/app/node_main.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/common/content_switches.h"
#include "electron/electron_version.h"
#include "gin/array_buffer.h"
#include "gin/public/isolate_holder.h"
#include "gin/v8_initializer.h"
#include "shell/app/uv_task_runner.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"

#if defined(OS_LINUX)
#include "components/crash/core/app/breakpad_linux.h"
#endif

#if defined(OS_WIN)
#include "chrome/child/v8_crashpad_support_win.h"
#endif

#if !defined(MAS_BUILD)
#include "components/crash/core/app/crashpad.h"  // 点名检查。
#include "shell/app/electron_crash_reporter_client.h"
#include "shell/browser/api/electron_api_crash_reporter.h"
#include "shell/common/crash_keys.h"
#endif

namespace {

// 初始化Node.js cli选项以传递给Node.js。
// 请参阅https://nodejs.org/api/cli.html#cli_options。
int SetNodeCliFlags() {
  // 单方面不允许的期权。
  const std::unordered_set<base::StringPiece, base::StringPieceHash>
      disallowed = {"--openssl-config", "--use-bundled-ca", "--use-openssl-ca",
                    "--force-fips", "--enable-fips"};

  const auto argv = base::CommandLine::ForCurrentProcess()->argv();
  std::vector<std::string> args;

  // TODO(Codebytere)：我们需要将args中的第一个条目设置为。
  // 由于src/node_options-inl.h#L286-L290而导致的进程名称，但这是。
  // 这是多余的，因此应该在上游进行重构。
  args.reserve(argv.size() + 1);
  args.emplace_back("electron");

  for (const auto& arg : argv) {
#if defined(OS_WIN)
    const auto& option = base::WideToUTF8(arg);
#else
    const auto& option = arg;
#endif
    const auto stripped = base::StringPiece(option).substr(0, option.find('='));
    if (disallowed.count(stripped) != 0) {
      LOG(ERROR) << "The Node.js cli flag " << stripped
                 << " is not supported in Electron";
      // 对于遇到的任何错误，Node.js从ProcessGlobalArgs返回9。
      // 在设置CLI标志和环境变量时。既然我们宣布这些都是非法的。
      // 为了保持一致性，标志(使其出错)在这里返回9。
      return 9;
    } else {
      args.push_back(option);
    }
  }

  std::vector<std::string> errors;

  // Node.js本身会将解析错误输出到。
  // 控制台，所以我们不需要自己处理。
  return ProcessGlobalArgs(&args, nullptr, &errors,
                           node::kDisallowedInEnvironment);
}

#if defined(MAS_BUILD)
void SetCrashKeyStub(const std::string& key, const std::string& value) {}
void ClearCrashKeyStub(const std::string& key) {}
#endif

}  // 命名空间。

namespace electron {

#if defined(OS_LINUX)
void CrashReporterStart(gin_helper::Dictionary options) {
  std::string submit_url;
  bool upload_to_server = true;
  bool ignore_system_crash_handler = false;
  bool rate_limit = false;
  bool compress = false;
  std::map<std::string, std::string> global_extra;
  std::map<std::string, std::string> extra;
  options.Get("submitURL", &submit_url);
  options.Get("uploadToServer", &upload_to_server);
  options.Get("ignoreSystemCrashHandler", &ignore_system_crash_handler);
  options.Get("rateLimit", &rate_limit);
  options.Get("compress", &compress);
  options.Get("extra", &extra);
  options.Get("globalExtra", &global_extra);

  std::string product_name;
  if (options.Get("productName", &product_name))
    global_extra["_productName"] = product_name;
  std::string company_name;
  if (options.Get("companyName", &company_name))
    global_extra["_companyName"] = company_name;
  api::crash_reporter::Start(submit_url, upload_to_server,
                             ignore_system_crash_handler, rate_limit, compress,
                             global_extra, extra, true);
}
#endif

v8::Local<v8::Value> GetParameters(v8::Isolate* isolate) {
  std::map<std::string, std::string> keys;
#if !defined(MAS_BUILD)
  electron::crash_keys::GetCrashKeys(&keys);
#endif
  return gin::ConvertToV8(isolate, keys);
}

int NodeMain(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);

#if defined(OS_WIN)
  v8_crashpad_support::SetUp();
#endif

#if !defined(MAS_BUILD)
  ElectronCrashReporterClient::Create();
#endif

#if defined(OS_WIN) || (defined(OS_MAC) && !defined(MAS_BUILD))
  crash_reporter::InitializeCrashpad(false, "node");
#endif

#if !defined(MAS_BUILD)
  crash_keys::SetCrashKeysFromCommandLine(
      *base::CommandLine::ForCurrentProcess());
  crash_keys::SetPlatformCrashKey();
#endif

  int exit_code = 1;
  {
    // 向GIN：：PerIsolateData提供任务运行器。
    uv_loop_t* loop = uv_default_loop();
    auto uv_task_runner = base::MakeRefCounted<UvTaskRunner>(loop);
    base::ThreadTaskRunnerHandle handle(uv_task_runner);

    // 初始化功能列表。
    auto feature_list = std::make_unique<base::FeatureList>();
    feature_list->InitializeFromCommandLine("", "");
    base::FeatureList::SetInstance(std::move(feature_list));

    // 明确注册电子的内置模块。
    NodeBindings::RegisterBuiltinModules();

    // 解析并设置Node.js cli标志。
    int flags_exit_code = SetNodeCliFlags();
    if (flags_exit_code != 0)
      exit(flags_exit_code);

    node::InitializationSettingsFlags flags = node::kRunPlatformInit;
    node::InitializationResult result =
        node::InitializeOncePerProcess(argc, argv, flags);

    if (result.early_return)
      exit(result.exit_code);

    gin::V8Initializer::LoadV8Snapshot(
        gin::V8Initializer::V8SnapshotFileType::kWithAdditionalContext);

    // V8需要一个任务调度器。
    base::ThreadPoolInstance::CreateAndStartWithDefaultParams("Electron");

    // 允许Node.js跟踪事件循环花费的时间。
    // 在内核的事件提供程序中处于空闲状态。
    uv_loop_configure(loop, UV_METRICS_IDLE_TIME);

    // 初始化gin：：IsolateHolder。
    JavascriptEnvironment gin_env(loop);

    v8::Isolate* isolate = gin_env.isolate();

    v8::Isolate::Scope isolate_scope(isolate);
    v8::Locker locker(isolate);
    node::Environment* env = nullptr;
    node::IsolateData* isolate_data = nullptr;
    {
      v8::HandleScope scope(isolate);

      isolate_data = node::CreateIsolateData(isolate, loop, gin_env.platform());
      CHECK_NE(nullptr, isolate_data);

      env = node::CreateEnvironment(isolate_data, gin_env.context(),
                                    result.args, result.exec_args);
      CHECK_NE(nullptr, env);

      node::IsolateSettings is;
      node::SetIsolateUpForNode(isolate, is);

      gin_helper::Dictionary process(isolate, env->process_object());
      process.SetMethod("crash", &ElectronBindings::Crash);

      // 设置进程。子节点进程中的crashReporter。
      gin_helper::Dictionary reporter = gin::Dictionary::CreateEmpty(isolate);
#if defined(OS_LINUX)
      reporter.SetMethod("start", &CrashReporterStart);
#endif

      reporter.SetMethod("getParameters", &GetParameters);
#if defined(MAS_BUILD)
      reporter.SetMethod("addExtraParameter", &SetCrashKeyStub);
      reporter.SetMethod("removeExtraParameter", &ClearCrashKeyStub);
#else
      reporter.SetMethod("addExtraParameter",
                         &electron::crash_keys::SetCrashKey);
      reporter.SetMethod("removeExtraParameter",
                         &electron::crash_keys::ClearCrashKey);
#endif

      process.Set("crashReporter", reporter);

      gin_helper::Dictionary versions;
      if (process.Get("versions", &versions)) {
        versions.SetReadOnly(ELECTRON_PROJECT_NAME, ELECTRON_VERSION_STRING);
      }
    }

    v8::HandleScope scope(isolate);
    node::LoadEnvironment(env, node::StartExecutionCallback{});

    env->set_trace_sync_io(env->options()->trace_sync_io);

    {
      v8::SealHandleScope seal(isolate);
      bool more;
      env->performance_state()->Mark(
          node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_START);
      do {
        uv_run(env->event_loop(), UV_RUN_DEFAULT);

        gin_env.platform()->DrainTasks(isolate);

        more = uv_loop_alive(env->event_loop());
        if (more && !env->is_stopping())
          continue;

        if (!uv_loop_alive(env->event_loop())) {
          EmitBeforeExit(env);
        }

        // 如果循环在发出后变为活动状态，则发出`beforeExit`。
        // 事件，或者在运行一些回调之后。
        more = uv_loop_alive(env->event_loop());
      } while (more && !env->is_stopping());
      env->performance_state()->Mark(
          node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_EXIT);
    }

    env->set_trace_sync_io(false);

    exit_code = node::EmitExit(env);

    node::ResetStdio();

    node::Stop(env);
    node::FreeEnvironment(env);
    node::FreeIsolateData(isolate_data);
  }

  // 根据“src/gin/shell/gin_main.cc”：
  // 
  // GIN：：IsolateHolder在其线程池中等待运行的任务。
  // 析构函数，因此必须在ThreadPool开始跳过之前销毁。
  // 继续关闭任务(_ON_SHUTDOWN)。
  base::ThreadPoolInstance::Get()->Shutdown();

  v8::V8::Dispose();

  return exit_code;
}

}  // 命名空间电子
