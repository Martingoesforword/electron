// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/node_bindings.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_paths.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/electron_command_line.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/locker.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "shell/common/mac/main_application_bundle.h"
#include "shell/common/node_includes.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_initializer.h"  // 点名检查。

#if !defined(MAS_BUILD)
#include "shell/common/crash_keys.h"
#endif

#define ELECTRON_BUILTIN_MODULES(V)      \
  V(electron_browser_app)                \
  V(electron_browser_auto_updater)       \
  V(electron_browser_browser_view)       \
  V(electron_browser_content_tracing)    \
  V(electron_browser_crash_reporter)     \
  V(electron_browser_dialog)             \
  V(electron_browser_event)              \
  V(electron_browser_event_emitter)      \
  V(electron_browser_global_shortcut)    \
  V(electron_browser_in_app_purchase)    \
  V(electron_browser_menu)               \
  V(electron_browser_message_port)       \
  V(electron_browser_net)                \
  V(electron_browser_power_monitor)      \
  V(electron_browser_power_save_blocker) \
  V(electron_browser_protocol)           \
  V(electron_browser_printing)           \
  V(electron_browser_safe_storage)       \
  V(electron_browser_session)            \
  V(electron_browser_system_preferences) \
  V(electron_browser_base_window)        \
  V(electron_browser_tray)               \
  V(electron_browser_view)               \
  V(electron_browser_web_contents)       \
  V(electron_browser_web_contents_view)  \
  V(electron_browser_web_frame_main)     \
  V(electron_browser_web_view_manager)   \
  V(electron_browser_window)             \
  V(electron_common_asar)                \
  V(electron_common_clipboard)           \
  V(electron_common_command_line)        \
  V(electron_common_environment)         \
  V(electron_common_features)            \
  V(electron_common_native_image)        \
  V(electron_common_native_theme)        \
  V(electron_common_notification)        \
  V(electron_common_screen)              \
  V(electron_common_shell)               \
  V(electron_common_v8_util)             \
  V(electron_renderer_context_bridge)    \
  V(electron_renderer_crash_reporter)    \
  V(electron_renderer_ipc)               \
  V(electron_renderer_web_frame)

#define ELECTRON_VIEWS_MODULES(V) V(electron_browser_image_view)

#define ELECTRON_DESKTOP_CAPTURER_MODULE(V) V(electron_browser_desktop_capturer)

#define ELECTRON_TESTING_MODULE(V) V(electron_common_testing)

// 这用于加载内置模块。而不是使用。
// __ATTRIBUTE__((构造函数))，我们调用_register_&lt;modname&gt;。
// 显式地为每个内置模块提供函数。这只是。
// 正向申报。定义在每个模块的。
// 调用NODE_LINKED_MODULE_CONTEXT_AWARE时实现。
#define V(modname) void _register_##modname();
ELECTRON_BUILTIN_MODULES(V)
#if BUILDFLAG(ENABLE_VIEWS_API)
ELECTRON_VIEWS_MODULES(V)
#endif
#if BUILDFLAG(ENABLE_DESKTOP_CAPTURER)
ELECTRON_DESKTOP_CAPTURER_MODULE(V)
#endif
#if DCHECK_IS_ON()
ELECTRON_TESTING_MODULE(V)
#endif
#undef V

namespace {

void stop_and_close_uv_loop(uv_loop_t* loop) {
  uv_stop(loop);

  auto const ensure_closing = [](uv_handle_t* handle, void*) {
    // 我们应该在任何地方使用UvHandle包装器，在这种情况下。
    // 所有手柄应已处于关闭状态...。
    DCHECK(uv_is_closing(handle));
    // .但是如果一个未加工的把手通过了，通过了，还是要做正确的事情。
    if (!uv_is_closing(handle)) {
      uv_close(handle, nullptr);
    }
  };

  uv_walk(loop, ensure_closing, nullptr);

  // 所有剩余的句柄现在都处于关闭状态。
  // 泵送事件循环，以便它们可以完成关闭。
  for (;;)
    if (uv_run(loop, UV_RUN_DEFAULT) == 0)
      break;

  DCHECK_EQ(0, uv_loop_alive(loop));
}

bool g_is_initialized = false;

void V8FatalErrorCallback(const char* location, const char* message) {
  LOG(ERROR) << "Fatal error in V8: " << location << " " << message;

#if !defined(MAS_BUILD)
  electron::crash_keys::SetCrashKey("electron.v8-fatal.message", message);
  electron::crash_keys::SetCrashKey("electron.v8-fatal.location", location);
#endif

  volatile int* zero = nullptr;
  *zero = 0;
}

bool AllowWasmCodeGenerationCallback(v8::Local<v8::Context> context,
                                     v8::Local<v8::String>) {
  // 如果我们在渲染器进程中启用了contextIsolation，
  // 回到布林克的逻辑。
  v8::Isolate* isolate = context->GetIsolate();
  if (node::Environment::GetCurrent(isolate) == nullptr) {
    if (gin_helper::Locker::IsBrowserProcess())
      return false;
    return blink::V8Initializer::WasmCodeGenerationCheckCallbackInMainThread(
        context, v8::String::Empty(isolate));
  }

  return node::AllowWasmCodeGenerationCallback(context,
                                               v8::String::Empty(isolate));
}

void ErrorMessageListener(v8::Local<v8::Message> message,
                          v8::Local<v8::Value> data) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  gin_helper::MicrotasksScope microtasks_scope(
      isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);
  node::Environment* env = node::Environment::GetCurrent(isolate);

  if (env) {
    // 既然异常已经被处理，就发出After()钩子。
    // 类似于node/lib/internal/process/execution.js#L176-L180。
    if (env->async_hooks()->fields()[node::AsyncHooks::kAfter]) {
      while (env->async_hooks()->fields()[node::AsyncHooks::kStackLength]) {
        node::AsyncWrap::EmitAfter(env, env->execution_async_id());
        env->async_hooks()->pop_async_context(env->execution_async_id());
      }
    }

    // 确保正确清除异步ID堆栈，以便异步。
    // 挂钩堆栈未损坏。
    env->async_hooks()->clear_async_id_stack();
  }
}

const std::unordered_set<base::StringPiece, base::StringPieceHash>
GetAllowedDebugOptions() {
  if (electron::fuses::IsNodeCliInspectEnabled()) {
    // 仅允许非电子Run_As_Node模式下的DebugOptions。
    return {
        "--inspect",          "--inspect-brk",
        "--inspect-port",     "--debug",
        "--debug-brk",        "--debug-port",
        "--inspect-brk-node", "--inspect-publish-uid",
    };
  }
  // 如果禁用了节点CLI检查支持，则不允许任何调试选项。
  return {};
}

// 初始化Node.js cli选项以传递给Node.js。
// 请参阅https://nodejs.org/api/cli.html#cli_options。
void SetNodeCliFlags() {
  const std::unordered_set<base::StringPiece, base::StringPieceHash> allowed =
      GetAllowedDebugOptions();

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

    // 仅允许在no-op(--)选项或DebugOptions中使用。
    if (allowed.count(stripped) != 0 || stripped == "--")
      args.push_back(option);
  }

  std::vector<std::string> errors;
  const int exit_code = ProcessGlobalArgs(&args, nullptr, &errors,
                                          node::kDisallowedInEnvironment);

  const std::string err_str = "Error parsing Node.js cli flags ";
  if (exit_code != 0) {
    LOG(ERROR) << err_str;
  } else if (!errors.empty()) {
    LOG(ERROR) << err_str << base::JoinString(errors, " ");
  }
}

// 初始化node_options以传递给Node.js。
// 请参阅https://nodejs.org/api/cli.html#cli_node_options_options。
void SetNodeOptions(base::Environment* env) {
  // 单方面不允许的期权。
  const std::set<std::string> disallowed = {
      "--openssl-config", "--use-bundled-ca", "--use-openssl-ca",
      "--force-fips", "--enable-fips"};

  // 打包应用程序中允许的选项子集。
  const std::set<std::string> allowed_in_packaged = {"--max-http-header-size",
                                                     "--http-parser"};

  if (env->HasVar("NODE_OPTIONS")) {
    if (electron::fuses::IsNodeOptionsEnabled()) {
      std::string options;
      env->GetVar("NODE_OPTIONS", &options);
      std::vector<std::string> parts = base::SplitString(
          options, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

      bool is_packaged_app = electron::api::App::IsPackaged();

      for (const auto& part : parts) {
        // 剥离传递给各个node_options的值。
        std::string option = part.substr(0, part.find('='));

        if (is_packaged_app &&
            allowed_in_packaged.find(option) == allowed_in_packaged.end()) {
          // 明确禁止打包应用程序中的大多数NODE_OPTIONS。
          LOG(ERROR) << "Most NODE_OPTIONs are not supported in packaged apps."
                     << " See documentation for more details.";
          options.erase(options.find(option), part.length());
        } else if (disallowed.find(option) != disallowed.end()) {
          // 删除专门不允许在Node.js中使用的node_options。
          // 由于像BoringSSL这样的限制，我们通过Electron。
          LOG(ERROR) << "The NODE_OPTION " << option
                     << " is not supported in Electron";
          options.erase(options.find(option), part.length());
        }
      }

      // 覆盖不带不支持变量的新NODE_OPTIONS。
      env->SetVar("NODE_OPTIONS", options);
    } else {
      LOG(ERROR) << "NODE_OPTIONS have been disabled in this app";
      env->SetVar("NODE_OPTIONS", "");
    }
  }
}

}  // 命名空间。

namespace electron {

namespace {

base::FilePath GetResourcesPath() {
#if defined(OS_MAC)
  return MainApplicationBundlePath().Append("Contents").Append("Resources");
#else
  auto* command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath exec_path(command_line->GetProgram());
  base::PathService::Get(base::FILE_EXE, &exec_path);

  return exec_path.DirName().Append(FILE_PATH_LITERAL("resources"));
#endif
}

}  // 命名空间。

NodeBindings::NodeBindings(BrowserEnvironment browser_env)
    : browser_env_(browser_env) {
  if (browser_env == BrowserEnvironment::kWorker) {
    uv_loop_init(&worker_loop_);
    uv_loop_ = &worker_loop_;
  } else {
    uv_loop_ = uv_default_loop();
  }
}

NodeBindings::~NodeBindings() {
  // 退出嵌入线程。
  embed_closed_ = true;
  uv_sem_post(&embed_sem_);

  WakeupEmbedThread();

  // 等待一切都做好。
  uv_thread_join(&embed_thread_);

  // 清除UV。
  uv_sem_destroy(&embed_sem_);
  dummy_uv_handle_.reset();

  // 清理工作循环
  if (in_worker_loop())
    stop_and_close_uv_loop(uv_loop_);
}

void NodeBindings::RegisterBuiltinModules() {
#define V(modname) _register_##modname();
  ELECTRON_BUILTIN_MODULES(V)
#if BUILDFLAG(ENABLE_VIEWS_API)
  ELECTRON_VIEWS_MODULES(V)
#endif
#if BUILDFLAG(ENABLE_DESKTOP_CAPTURER)
  ELECTRON_DESKTOP_CAPTURER_MODULE(V)
#endif
#if DCHECK_IS_ON()
  ELECTRON_TESTING_MODULE(V)
#endif
#undef V
}

bool NodeBindings::IsInitialized() {
  return g_is_initialized;
}

void NodeBindings::Initialize() {
  TRACE_EVENT0("electron", "NodeBindings::Initialize");
  // 开放节点的错误报告系统，供浏览器进程使用。
  node::g_upstream_node_mode = false;

#if defined(OS_LINUX)
  // 获取由zygote派生的渲染器进程中的真实命令行。
  if (browser_env_ != BrowserEnvironment::kBrowser)
    ElectronCommandLine::InitializeFromCommandLine();
#endif

  // 明确注册电子的内置模块。
  RegisterBuiltinModules();

  // 解析并设置Node.js cli标志。
  SetNodeCliFlags();

  auto env = base::Environment::Create();
  SetNodeOptions(env.get());
  node::Environment::should_read_node_options_from_env_ =
      fuses::IsNodeOptionsEnabled();

  std::vector<std::string> argv = {"electron"};
  std::vector<std::string> exec_argv;
  std::vector<std::string> errors;

  int exit_code = node::InitializeNodeWithArgs(&argv, &exec_argv, &errors);

  for (const std::string& error : errors)
    fprintf(stderr, "%s: %s\n", argv[0].c_str(), error.c_str());

  if (exit_code != 0)
    exit(exit_code);

#if defined(OS_WIN)
  // Uv_init覆盖错误模式以取消默认的崩溃对话框。
  // 如果用户想要显示它，它会返回。
  if (browser_env_ == BrowserEnvironment::kBrowser ||
      env->HasVar("ELECTRON_DEFAULT_ERROR_MODE"))
    SetErrorMode(GetErrorMode() & ~SEM_NOGPFAULTERRORBOX);
#endif

  g_is_initialized = true;
}

node::Environment* NodeBindings::CreateEnvironment(
    v8::Handle<v8::Context> context,
    node::MultiIsolatePlatform* platform) {
#if defined(OS_WIN)
  auto& atom_args = ElectronCommandLine::argv();
  std::vector<std::string> args(atom_args.size());
  std::transform(atom_args.cbegin(), atom_args.cend(), args.begin(),
                 [](auto& a) { return base::WideToUTF8(a); });
#else
  auto args = ElectronCommandLine::argv();
#endif

  // Feed节点初始化脚本的路径。
  std::string process_type;
  switch (browser_env_) {
    case BrowserEnvironment::kBrowser:
      process_type = "browser";
      break;
    case BrowserEnvironment::kRenderer:
      process_type = "renderer";
      break;
    case BrowserEnvironment::kWorker:
      process_type = "worker";
      break;
  }

  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary global(isolate, context->Global());
  // 不要为渲染器进程设置DOM全局变量。
  // 我们必须在内部运行的节点引导程序之前设置此设置。
  // CreateEnvironment。
  if (browser_env_ != BrowserEnvironment::kBrowser)
    global.Set("_noBrowserGlobals", true);

  if (browser_env_ == BrowserEnvironment::kBrowser) {
    const std::vector<std::string> search_paths = {"app.asar", "app",
                                                   "default_app.asar"};
    const std::vector<std::string> app_asar_search_paths = {"app.asar"};
    context->Global()->SetPrivate(
        context,
        v8::Private::ForApi(
            isolate,
            gin::ConvertToV8(isolate, "appSearchPaths").As<v8::String>()),
        gin::ConvertToV8(isolate,
                         electron::fuses::IsOnlyLoadAppFromAsarEnabled()
                             ? app_asar_search_paths
                             : search_paths));
  }

  std::vector<std::string> exec_args;
  base::FilePath resources_path = GetResourcesPath();
  std::string init_script = "electron/js2c/" + process_type + "_init";

  args.insert(args.begin() + 1, init_script);

  isolate_data_ =
      node::CreateIsolateData(context->GetIsolate(), uv_loop_, platform);

  node::Environment* env;
  uint64_t flags = node::EnvironmentFlags::kDefaultFlags |
                   node::EnvironmentFlags::kHideConsoleWindows |
                   node::EnvironmentFlags::kNoGlobalSearchPaths;

  if (browser_env_ != BrowserEnvironment::kBrowser) {
    // 每个隔离物只能注册一个ESM加载器-。
    // 在渲染器进程中，这应该是闪烁的。我们需要告诉Node.js。
    // 不在非浏览器进程中注册其处理程序(覆盖闪烁)。
    flags |= node::EnvironmentFlags::kNoRegisterESMLoader |
             node::EnvironmentFlags::kNoInitializeInspector;
    v8::TryCatch try_catch(context->GetIsolate());
    env = node::CreateEnvironment(
        isolate_data_, context, args, exec_args,
        static_cast<node::EnvironmentFlags::Flags>(flags));
    DCHECK(env);

    // 只有当某些事情出了可怕的问题时，才会发现这一点。
    // 电子剧本包装在webpack的一次尝试{}捕捉{}中。
    if (try_catch.HasCaught()) {
      LOG(ERROR) << "Failed to initialize node environment in process: "
                 << process_type;
    }
  } else {
    env = node::CreateEnvironment(
        isolate_data_, context, args, exec_args,
        static_cast<node::EnvironmentFlags::Flags>(flags));
    DCHECK(env);
  }

  // 清理我们以非讽刺方式注入的global_noBrowserGlobals。
  // 全球范围。
  if (browser_env_ != BrowserEnvironment::kBrowser) {
    // 我们需要在非浏览器进程中引导env，以便。
    // _noBrowserGlobals在我们删除之前已正确读取。
    global.Delete("_noBrowserGlobals");
  }

  node::IsolateSettings is;

  // 使用自定义致命错误回调允许我们添加。
  // 将崩溃消息和位置发送到CrashReports。
  is.fatal_error_callback = V8FatalErrorCallback;

  // 我们不想在渲染器或浏览器进程中中止。
  // 我们已经在监听未捕获的异常并在那里处理它们。
  is.should_abort_on_uncaught_exception_callback = [](v8::Isolate*) {
    return false;
  };

  // 在此处使用自定义回调，以允许我们在。
  // 渲染器进程。
  is.allow_wasm_code_generation_callback = AllowWasmCodeGenerationCallback;

  if (browser_env_ == BrowserEnvironment::kBrowser) {
    // Js要求显式调用微任务检查点。
    is.policy = v8::MicrotasksPolicy::kExplicit;
  } else {
    // Blink期望微任务策略是kScoped，但Node.js期望它。
    // 去做一个kexplexent。在呈现器中，
    // 相同的隔离，所以我们不想在这里更改现有策略，这。
    // 可以是kExplative或kScoped，具体取决于我们是否正在执行。
    // 从Node.js或Blink入口点中。相反，政策是。
    // 已在通过UvRunOnce输入Node.js时切换为kExpltual。
    is.policy = context->GetIsolate()->GetMicrotasksPolicy();

    // 我们不想使用Node.js的消息监听器，因为它会干扰。
    // 眨眼的。
    is.flags &= ~node::IsolateSettingsFlags::MESSAGE_LISTENER_WITH_ERROR_LEVEL;

    // 隔离消息监听器是累加的(您可以添加多个)，因此。
    // 我们在这里添加了一个额外的钩子堆栈，以确保异步挂接堆栈正确。
    // 引发错误时清除。
    context->GetIsolate()->AddMessageListenerWithErrorLevel(
        ErrorMessageListener, v8::Isolate::kMessageError);

    // 我们不想使用Node.js使用的Promise Rejection回调。
    // 因为它不向全局脚本发送PromiseRejectionEvents。
    // 背景。我们需要使用Blink已经提供的功能。
    is.flags |=
        node::IsolateSettingsFlags::SHOULD_NOT_SET_PROMISE_REJECTION_CALLBACK;

    // 我们不想使用Node.js使用的堆栈跟踪回调，
    // 因为它依赖于Node.js感知当前上下文，并且。
    // 情况并不总是这样。我们已经需要使用One Blink了。
    // 提供了。
    is.flags |=
        node::IsolateSettingsFlags::SHOULD_NOT_SET_PREPARE_STACK_TRACE_CALLBACK;
  }

  node::SetIsolateUpForNode(context->GetIsolate(), is);

  gin_helper::Dictionary process(context->GetIsolate(), env->process_object());
  process.SetReadOnly("type", process_type);
  process.Set("resourcesPath", resources_path);
  // 助手应用程序的路径。
  base::FilePath helper_exec_path;
  base::PathService::Get(content::CHILD_PROCESS_EXE, &helper_exec_path);
  process.Set("helperExecPath", helper_exec_path);

  return env;
}

void NodeBindings::LoadEnvironment(node::Environment* env) {
  node::LoadEnvironment(env, node::StartExecutionCallback{});
  gin_helper::EmitEvent(env->isolate(), env->process_object(), "loaded");
}

void NodeBindings::PrepareMessageLoop() {
#if !defined(OS_WIN)
  int handle = uv_backend_fd(uv_loop_);

  // 如果后端FD没有更改，请不要继续。
  if (handle == handle_)
    return;

  handle_ = handle;
#endif

  // 为libuv添加虚拟句柄，否则libuv将在存在时退出。
  // 没什么可做的。
  uv_async_init(uv_loop_, dummy_uv_handle_.get(), nullptr);

  // 启动发生UV事件时将中断主循环的Worker。
  uv_sem_init(&embed_sem_, 0);
  uv_thread_create(&embed_thread_, EmbedThreadRunner, this);
}

void NodeBindings::RunMessageLoop() {
  // 应该已经创建了MessageLoop，请记住主线程中的那个。
  task_runner_ = base::ThreadTaskRunnerHandle::Get();

  // 运行一次UV循环，使uv__io_poll有机会添加所有事件。
  UvRunOnce();
}

void NodeBindings::UvRunOnce() {
  node::Environment* env = uv_env();

  // 在不重新启动渲染器进程的情况下执行导航时，可能会发生这种情况。
  // 节点环境被破坏，但消息循环仍然存在。
  // 在这种情况下，我们不应该运行UV循环。
  if (!env)
    return;

  // 在浏览器进程中使用锁定器。
  gin_helper::Locker locker(env->isolate());
  v8::HandleScope handle_scope(env->isolate());

  // 在处理UV事件时输入节点上下文。
  v8::Context::Scope context_scope(env->context());

  // Node.js需要`kExplitt`微任务策略，并将运行微任务。
  // 每次调用JavaScript后设置检查点。因为我们使用了不同的。
  // 渲染器中的策略-切换到`kExplit`，然后放回。
  // 以前的策略值。
  auto old_policy = env->isolate()->GetMicrotasksPolicy();
  DCHECK_EQ(v8::MicrotasksScope::GetCurrentDepth(env->isolate()), 0);
  env->isolate()->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);

  if (browser_env_ != BrowserEnvironment::kBrowser)
    TRACE_EVENT_BEGIN0("devtools.timeline", "FunctionCall");

  // 处理紫外线事件。
  int r = uv_run(uv_loop_, UV_RUN_NOWAIT);

  if (browser_env_ != BrowserEnvironment::kBrowser)
    TRACE_EVENT_END0("devtools.timeline", "FunctionCall");

  env->isolate()->SetMicrotasksPolicy(old_policy);

  if (r == 0)
    base::RunLoop().QuitWhenIdle();  // 退出UV。

  // 告诉工作线程继续轮询。
  uv_sem_post(&embed_sem_);
}

void NodeBindings::WakeupMainThread() {
  DCHECK(task_runner_);
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&NodeBindings::UvRunOnce,
                                                   weak_factory_.GetWeakPtr()));
}

void NodeBindings::WakeupEmbedThread() {
  uv_async_send(dummy_uv_handle_.get());
}

// 静电。
void NodeBindings::EmbedThreadRunner(void* arg) {
  auto* self = static_cast<NodeBindings*>(arg);

  while (true) {
    // 等待主循环处理事件。
    uv_sem_wait(&self->embed_sem_);
    if (self->embed_closed_)
      break;

    // 等待UV循环中发生的事情。
    // 请注意，PollEvents()是由派生类实现的，因此当。
    // 此类正在被析构，PollEvents()将不可用。
    // 更多。因此，我们必须确保只调用PollEvents()。
    // 当这个班级还活着的时候。
    self->PollEvents();
    if (self->embed_closed_)
      break;

    // 在主线程中处理事件。
    self->WakeupMainThread();
  }
}

}  // 命名空间电子
