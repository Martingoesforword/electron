// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/electron_browser_main_parts.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/icon_manager.h"
#include "components/os_crypt/os_crypt.h"
#include "content/browser/browser_main_loop.h"  // 点名检查。
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/device_service.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "media/base/localized_strings.h"
#include "services/network/public/cpp/features.h"
#include "services/tracing/public/cpp/stack_sampling/tracing_sampler_profiler.h"
#include "shell/app/electron_main_delegate.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/browser/browser.h"
#include "shell/browser/browser_process_impl.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/electron_web_ui_controller_factory.h"
#include "shell/browser/feature_list.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/media/media_capture_devices_dispatcher.h"
#include "shell/browser/ui/devtools_manager_delegate.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/application_info.h"
#include "shell/common/electron_paths.h"
#include "shell/common/gin_helper/trackable_object.h"
#include "shell/common/logging.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/idle/idle.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_switches.h"

#if defined(USE_AURA)
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/wm/core/wm_state.h"
#endif

#if defined(OS_LINUX)
#include "base/environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/gtk/gtk_ui_factory.h"
#include "ui/gtk/gtk_util.h"
#include "ui/views/linux_ui/linux_ui.h"

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif

#if defined(USE_X11)
#include "ui/base/x/x11_util.h"
#include "ui/events/devices/x11/touch_factory_x11.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/x/connection.h"
#include "ui/gfx/x/xproto_util.h"
#endif

#if defined(USE_OZONE) || defined(USE_X11)
#include "ui/base/ui_base_features.h"
#endif

#endif

#if defined(OS_WIN)
#include "ui/base/l10n/l10n_util_win.h"
#include "ui/display/win/dpi.h"
#include "ui/gfx/system_fonts_win.h"
#include "ui/strings/grit/app_locale_settings.h"
#endif

#if defined(OS_MAC)
#include "services/device/public/cpp/geolocation/geolocation_manager.h"
#include "shell/browser/ui/cocoa/views_delegate_mac.h"
#else
#include "shell/browser/ui/views/electron_views_delegate.h"
#endif

#if defined(OS_LINUX)
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/dbus/dbus_bluez_manager_wrapper_linux.h"
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/browser_context_keyed_service_factories.h"
#include "extensions/common/extension_api.h"
#include "shell/browser/extensions/electron_browser_context_keyed_service_factories.h"
#include "shell/browser/extensions/electron_extensions_browser_client.h"
#include "shell/common/extensions/electron_extensions_client.h"
#endif  // BUILDFLAG(启用电子扩展)。

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "chrome/browser/spellchecker/spellcheck_factory.h"  // 点名检查。
#endif

namespace electron {

namespace {

template <typename T>
void Erase(T* container, typename T::iterator iter) {
  container->erase(iter);
}

#if defined(OS_WIN)
// GFX：：Font回调。
void AdjustUIFont(gfx::win::FontAdjustment* font_adjustment) {
  l10n_util::NeedOverrideDefaultUIFont(&font_adjustment->font_family_override,
                                       &font_adjustment->font_scale);
  font_adjustment->font_scale *= display::win::GetAccessibilityFontScale();
}

int GetMinimumFontSize() {
  int min_font_size;
  base::StringToInt(l10n_util::GetStringUTF16(IDS_MINIMUM_UI_FONT_SIZE),
                    &min_font_size);
  return min_font_size;
}
#endif

std::u16string MediaStringProvider(media::MessageId id) {
  switch (id) {
    case media::DEFAULT_AUDIO_DEVICE_NAME:
      return u"Default";
#if defined(OS_WIN)
    case media::COMMUNICATIONS_AUDIO_DEVICE_NAME:
      return u"Communications";
#endif
    default:
      return std::u16string();
  }
}

#if defined(OS_LINUX)
// GTK不提供检查当前主题是否暗的方法，所以我们比较。
// 文本和背景亮度以获得结果。
// 这个技巧来自Firefox。
void UpdateDarkThemeSetting() {
  float bg = color_utils::GetRelativeLuminance(gtk::GetBgColor("GtkLabel"));
  float fg = color_utils::GetRelativeLuminance(gtk::GetFgColor("GtkLabel"));
  bool is_dark = fg > bg;
  // 将其传递给nativeUi主题，该主题由nativeTheme模块和MOST。
  // 在电子公司的位置。
  ui::NativeTheme::GetInstanceForNativeUi()->set_use_dark_colors(is_dark);
  // 传给Web Theme，让“偏好配色方案”的媒体查询工作。
  ui::NativeTheme::GetInstanceForWeb()->set_use_dark_colors(is_dark);
}
#endif

}  // 命名空间。

#if defined(OS_LINUX)
class DarkThemeObserver : public ui::NativeThemeObserver {
 public:
  DarkThemeObserver() = default;

  // UI：：NativeThemeViewer：
  void OnNativeThemeUpdated(ui::NativeTheme* observed_theme) override {
    UpdateDarkThemeSetting();
  }
};
#endif

// 静电。
ElectronBrowserMainParts* ElectronBrowserMainParts::self_ = nullptr;

ElectronBrowserMainParts::ElectronBrowserMainParts(
    const content::MainFunctionParams& params)
    : fake_browser_process_(std::make_unique<BrowserProcessImpl>()),
      browser_(std::make_unique<Browser>()),
      node_bindings_(
          NodeBindings::Create(NodeBindings::BrowserEnvironment::kBrowser)),
      electron_bindings_(
          std::make_unique<ElectronBindings>(node_bindings_->uv_loop())) {
  DCHECK(!self_) << "Cannot have two ElectronBrowserMainParts";
  self_ = this;
}

ElectronBrowserMainParts::~ElectronBrowserMainParts() = default;

// 静电。
ElectronBrowserMainParts* ElectronBrowserMainParts::Get() {
  DCHECK(self_);
  return self_;
}

bool ElectronBrowserMainParts::SetExitCode(int code) {
  if (!exit_code_)
    return false;

  content::BrowserMainLoop::GetInstance()->SetResultCode(code);
  *exit_code_ = code;
  return true;
}

int ElectronBrowserMainParts::GetExitCode() const {
  return exit_code_.value_or(content::RESULT_CODE_NORMAL_EXIT);
}

int ElectronBrowserMainParts::PreEarlyInitialization() {
  field_trial_list_ = std::make_unique<base::FieldTrialList>(nullptr);
#if defined(OS_POSIX)
  HandleSIGCHLD();
#endif

  return GetExitCode();
}

void ElectronBrowserMainParts::PostEarlyInitialization() {
  // 之前需要解决方法，因为没有ThreadTaskRunner。
  // 准备好了。如果此检查失败，我们可能需要重新添加解决方法。
  DCHECK(base::ThreadTaskRunnerHandle::IsSet());

  // ProxyResolverV8设置了一个完整的V8环境，以便。
  // 避免冲突，我们只在那之后初始化我们的V8环境。
  js_env_ = std::make_unique<JavascriptEnvironment>(node_bindings_->uv_loop());

  v8::HandleScope scope(js_env_->isolate());

  node_bindings_->Initialize();
  // 营造全球环境。
  node::Environment* env = node_bindings_->CreateEnvironment(
      js_env_->context(), js_env_->platform());
  node_env_ = std::make_unique<NodeEnvironment>(env);

  env->set_trace_sync_io(env->options()->trace_sync_io);

  // 我们不想让未处理的拒绝导致主进程崩溃。
  env->options()->unhandled_rejections = "warn";

  // 添加电子扩展API。
  electron_bindings_->BindTo(js_env_->isolate(), env->process_object());

  // 把所有东西都装上。
  node_bindings_->LoadEnvironment(env);

  // 使用全局环境包裹UV循环。
  node_bindings_->set_uv_env(env);

  // 我们已经在PreEarlyInitialization()中初始化了功能列表，但是。
  // 用户JS脚本将没有机会更改命令行。
  // 在这一点上切换。让我们在这里重新初始化它，以获取。
  // 命令行更改。
  base::FeatureList::ClearInstanceForTesting();
  InitializeFeatureList();

  // 初始化现场试验。
  InitializeFieldTrials();

  // 现在应用程序已有机会设置应用程序名称，因此重新初始化日志记录。
  // 和/或用户数据目录。
  logging::InitElectronLogging(*base::CommandLine::ForCurrentProcess(),
                               /* IS_PREINIT=。*/ false);

  // 创建用户脚本环境后进行初始化。
  fake_browser_process_->PostEarlyInitialization();
}

int ElectronBrowserMainParts::PreCreateThreads() {
#if defined(USE_AURA)
  screen_ = views::CreateDesktopScreen();
  display::Screen::SetScreenInstance(screen_.get());
#if defined(OS_LINUX)
  views::LinuxUI::instance()->UpdateDeviceScaleFactor();
#endif
#endif

  if (!views::LayoutProvider::Get())
    layout_provider_ = std::make_unique<views::LayoutProvider>();

  auto* command_line = base::CommandLine::ForCurrentProcess();
  std::string locale = command_line->GetSwitchValueASCII(::switches::kLang);

#if defined(OS_MAC)
  // 浏览器进程只想支持Cocoa将使用的语言，
  // 因此，强制使用该值覆盖应用程序区域设置。这一定是。
  // 在加载ResourceBundle之前发生。
  if (locale.empty())
    l10n_util::OverrideLocaleWithCocoaLocale();
#elif defined(OS_LINUX)
  // L10n_util：：GetApplicationLocaleInternal Uses g_Get_Language_Names()，
  // 哪个键关闭getenv(“LC_ALL”)。
  // 我们必须首先设置此env以使UI：：ResourceBundle接受自定义。
  // 地点。
  auto env = base::Environment::Create();
  absl::optional<std::string> lc_all;
  if (!locale.empty()) {
    std::string str;
    if (env->GetVar("LC_ALL", &str))
      lc_all.emplace(std::move(str));
    env->SetVar("LC_ALL", locale.c_str());
  }
#endif

  // 根据区域设置加载资源包。
  std::string loaded_locale = LoadResourceBundle(locale);

  // 初始化应用程序区域设置。
  std::string app_locale = l10n_util::GetApplicationLocale(loaded_locale);
  ElectronBrowserClient::SetApplicationLocale(app_locale);
  fake_browser_process_->SetApplicationLocale(app_locale);

#if defined(OS_LINUX)
  // 重置为原始的LC_ALL，因为我们不应该更改它。
  if (!locale.empty()) {
    if (lc_all)
      env->SetVar("LC_ALL", *lc_all);
    else
      env->UnSetVar("LC_ALL");
  }
#endif

  // 强制在UI线程上创建MediaCaptureDevicesDispatcher。
  MediaCaptureDevicesDispatcher::GetInstance();

  // 强制在UI线程上创建MediaCaptureDevicesDispatcher。
  MediaCaptureDevicesDispatcher::GetInstance();

#if defined(OS_MAC)
  ui::InitIdleMonitor();
#endif

  fake_browser_process_->PreCreateThreads();

  // 通知观察员。
  Browser::Get()->PreCreateThreads();

  return 0;
}

void ElectronBrowserMainParts::PostCreateThreads() {
  base::PostTask(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&tracing::TracingSamplerProfiler::CreateOnChildThread));
}

void ElectronBrowserMainParts::PostDestroyThreads() {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_browser_client_.reset();
  extensions::ExtensionsBrowserClient::Set(nullptr);
#endif
#if defined(OS_LINUX)
  device::BluetoothAdapterFactory::Shutdown();
  bluez::DBusBluezManagerWrapperLinux::Shutdown();
#endif
  fake_browser_process_->PostDestroyThreads();
}

void ElectronBrowserMainParts::ToolkitInitialized() {
#if defined(OS_LINUX)
  auto linux_ui = BuildGtkUi();
  linux_ui->Initialize();

  // 铬不尊重GTK深色主题设置，但它们可能会改变。
  // 将来可能不再需要此代码。检查铬。
  // 要保持最新的问题：
  // Https://bugs.chromium.org/p/chromium/issues/detail?id=998903。
  UpdateDarkThemeSetting();
  // 当GTK主题更改时更新原生主题。GetNativeTheme。
  // 这里返回一个NativeThemeGtk，它监视GTK设置。
  dark_theme_observer_ = std::make_unique<DarkThemeObserver>();
  linux_ui->GetNativeTheme(nullptr)->AddObserver(dark_theme_observer_.get());
  views::LinuxUI::SetInstance(std::move(linux_ui));
#endif

#if defined(USE_AURA)
  wm_state_ = std::make_unique<wm::WMState>();
#endif

#if defined(OS_WIN)
  gfx::win::SetAdjustFontCallback(&AdjustUIFont);
  gfx::win::SetGetMinimumFontSizeCallback(&GetMinimumFontSize);
#endif

#if defined(OS_MAC)
  views_delegate_ = std::make_unique<ViewsDelegateMac>();
#else
  views_delegate_ = std::make_unique<ViewsDelegate>();
#endif
}

int ElectronBrowserMainParts::PreMainMessageLoopRun() {
  // 在大多数事情初始化之前运行用户的主脚本，这样我们就可以。
  // 一个把一切都安排好的机会。
  node_bindings_->PrepareMessageLoop();
  node_bindings_->RunMessageLoop();

  // URL：：Add*Scheme不是ThreadSafe，这有助于防止数据竞争。
  url::LockSchemeRegistries();

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_client_ = std::make_unique<ElectronExtensionsClient>();
  extensions::ExtensionsClient::Set(extensions_client_.get());

  // BrowserContextKeyedAPI服务工厂需要ExtensionsBrowserClient。
  extensions_browser_client_ =
      std::make_unique<ElectronExtensionsBrowserClient>();
  extensions::ExtensionsBrowserClient::Set(extensions_browser_client_.get());

  extensions::EnsureBrowserContextKeyedServiceFactoriesBuilt();
  extensions::electron::EnsureBrowserContextKeyedServiceFactoriesBuilt();
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  SpellcheckServiceFactory::GetInstance();
#endif

#if defined(USE_X11)
  ui::TouchFactory::SetTouchDeviceListFromCommandLine();
#endif

  content::WebUIControllerFactory::RegisterFactory(
      ElectronWebUIControllerFactory::GetInstance());

  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kRemoteDebuggingPipe)) {
    // --远程调试-管道。
    auto on_disconnect = base::BindOnce([]() {
      base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                     base::BindOnce([]() { Browser::Get()->Quit(); }));
    });
    content::DevToolsAgentHost::StartRemoteDebuggingPipeHandler(
        std::move(on_disconnect));
  } else if (command_line->HasSwitch(switches::kRemoteDebuggingPort)) {
    // --远程调试端口。
    DevToolsManagerDelegate::StartHttpHandler();
  }

#if !defined(OS_MAC)
  // MacOS中的相应调用在ElectronApplicationDelegate中。
  Browser::Get()->WillFinishLaunching();
  Browser::Get()->DidFinishLaunching(base::DictionaryValue());
#endif

  // 通知观察者主线程消息循环已初始化。
  Browser::Get()->PreMainMessageLoopRun();

  return GetExitCode();
}

void ElectronBrowserMainParts::WillRunMainMessageLoop(
    std::unique_ptr<base::RunLoop>& run_loop) {
  js_env_->OnMessageLoopCreated();
  exit_code_ = content::RESULT_CODE_NORMAL_EXIT;
  Browser::Get()->SetMainMessageLoopQuitClosure(run_loop->QuitClosure());
}

void ElectronBrowserMainParts::PostCreateMainMessageLoop() {
#if defined(USE_OZONE)
  if (features::IsUsingOzonePlatform()) {
    auto shutdown_cb =
        base::BindOnce(base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
    ui::OzonePlatform::GetInstance()->PostCreateMainMessageLoop(
        std::move(shutdown_cb));
  }
#endif
#if defined(OS_LINUX)
  bluez::DBusBluezManagerWrapperLinux::Initialize();
#endif
#if defined(OS_POSIX)
  // 响应SIGINT、SIGTERM等退出。
  InstallShutdownSignalHandlers(
      base::BindOnce(&Browser::Quit, base::Unretained(Browser::Get())),
      content::GetUIThreadTaskRunner({}));
#endif
}

void ElectronBrowserMainParts::PostMainMessageLoopRun() {
#if defined(OS_MAC)
  FreeAppDelegate();
#endif

  // 在销毁节点之前关闭DownloadManager以防止。
  // DownloadItem从崩溃中回调。
  for (auto& iter : ElectronBrowserContext::browser_context_map()) {
    auto* download_manager = iter.second.get()->GetDownloadManager();
    if (download_manager) {
      download_manager->Shutdown();
    }
  }

  // 在执行所有析构函数后销毁节点平台，视情况而定。
  // 调用其中的Node/v8API。
  node_env_->env()->set_trace_sync_io(false);
  js_env_->OnMessageLoopDestroying();
  node::Stop(node_env_->env());
  node_env_.reset();

  auto default_context_key = ElectronBrowserContext::PartitionKey("", false);
  std::unique_ptr<ElectronBrowserContext> default_context = std::move(
      ElectronBrowserContext::browser_context_map()[default_context_key]);
  ElectronBrowserContext::browser_context_map().clear();
  default_context.reset();

  fake_browser_process_->PostMainMessageLoopRun();
  content::DevToolsAgentHost::StopRemoteDebuggingPipeHandler();
}

#if !defined(OS_MAC)
void ElectronBrowserMainParts::PreCreateMainMessageLoop() {
  PreCreateMainMessageLoopCommon();
}
#endif

void ElectronBrowserMainParts::PreCreateMainMessageLoopCommon() {
#if defined(OS_MAC)
  InitializeMainNib();
  RegisterURLHandler();
#endif
  media::SetLocalizedStringProvider(MediaStringProvider);

#if defined(OS_WIN)
  auto* local_state = g_browser_process->local_state();
  DCHECK(local_state);

  bool os_crypt_init = OSCrypt::Init(local_state);
  DCHECK(os_crypt_init);
#endif
}

device::mojom::GeolocationControl*
ElectronBrowserMainParts::GetGeolocationControl() {
  if (!geolocation_control_) {
    content::GetDeviceService().BindGeolocationControl(
        geolocation_control_.BindNewPipeAndPassReceiver());
  }
  return geolocation_control_.get();
}

#if defined(OS_MAC)
device::GeolocationManager* ElectronBrowserMainParts::GetGeolocationManager() {
  return geolocation_manager_.get();
}
#endif

IconManager* ElectronBrowserMainParts::GetIconManager() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!icon_manager_.get())
    icon_manager_ = std::make_unique<IconManager>();
  return icon_manager_.get();
}

}  // 命名空间电子
