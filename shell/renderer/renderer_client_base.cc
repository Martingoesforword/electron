// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/renderer_client_base.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "components/network_hints/renderer/web_prescient_networking_impl.h"
#include "content/common/buildflags.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "electron/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "shell/browser/api/electron_api_protocol.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/options_switches.h"
#include "shell/common/world_ids.h"
#include "shell/renderer/api/context_bridge/object_cache.h"
#include "shell/renderer/api/electron_api_context_bridge.h"
#include "shell/renderer/browser_exposed_renderer_interfaces.h"
#include "shell/renderer/content_settings_observer.h"
#include "shell/renderer/electron_api_service_impl.h"
#include "shell/renderer/electron_autofill_agent.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/media/multi_buffer_data_source.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_custom_element.h"  // NOLINT(Build/Include_Alpha)。
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_plugin_params.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/public/web/web_security_policy.h"
#include "third_party/blink/public/web/web_view.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"  // 点名检查。

#if defined(OS_MAC)
#include "base/strings/sys_string_conversions.h"
#endif

#if defined(OS_WIN)
#include <shlobj.h>
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "components/spellcheck/renderer/spellcheck.h"
#include "components/spellcheck/renderer/spellcheck_provider.h"
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "shell/common/electron_constants.h"
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)。

#if BUILDFLAG(ENABLE_PLUGINS)
#include "shell/renderer/pepper_helper.h"
#endif  // BUILDFLAG(ENABLE_Plugin)。

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/printing/renderer/print_render_frame_helper.h"
#include "printing/print_settings.h"  // 点名检查。
#include "shell/renderer/printing/print_render_frame_helper_delegate.h"
#endif  // BUILDFLAG(ENABLE_PRINTING)。

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/webplugininfo.h"
#include "extensions/common/constants.h"
#include "extensions/common/extensions_client.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/extension_frame_helper.h"
#include "extensions/renderer/guest_view/mime_handler_view/mime_handler_view_container_manager.h"
#include "shell/common/extensions/electron_extensions_client.h"
#include "shell/renderer/extensions/electron_extensions_renderer_client.h"
#endif  // BUILDFLAG(启用电子扩展)。

namespace electron {

content::RenderFrame* GetRenderFrame(v8::Local<v8::Object> value);

namespace {

void SetIsWebView(v8::Isolate* isolate, v8::Local<v8::Object> object) {
  gin_helper::Dictionary dict(isolate, object);
  dict.SetHidden("isWebView", true);
}

std::vector<std::string> ParseSchemesCLISwitch(base::CommandLine* command_line,
                                               const char* switch_name) {
  std::string custom_schemes = command_line->GetSwitchValueASCII(switch_name);
  return base::SplitString(custom_schemes, ",", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

// 静电。
RendererClientBase* g_renderer_client_base = nullptr;

}  // 命名空间。

RendererClientBase::RendererClientBase() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  // Parse--service-worker-schemes=scheme1，Scheme2。
  std::vector<std::string> service_worker_schemes_list =
      ParseSchemesCLISwitch(command_line, switches::kServiceWorkerSchemes);
  for (const std::string& scheme : service_worker_schemes_list)
    electron::api::AddServiceWorkerScheme(scheme);
  // Parse--Standard-Schemes=scheme1，Scheme2。
  std::vector<std::string> standard_schemes_list =
      ParseSchemesCLISwitch(command_line, switches::kStandardSchemes);
  for (const std::string& scheme : standard_schemes_list)
    url::AddStandardScheme(scheme.c_str(), url::SCHEME_WITH_HOST);
  // Parse--CORS-SCHEMES=scheme1，Scheme2。
  std::vector<std::string> cors_schemes_list =
      ParseSchemesCLISwitch(command_line, switches::kCORSSchemes);
  for (const std::string& scheme : cors_schemes_list)
    url::AddCorsEnabledScheme(scheme.c_str());
  // Parse--Streaming-Schemes=scheme1，Scheme2。
  std::vector<std::string> streaming_schemes_list =
      ParseSchemesCLISwitch(command_line, switches::kStreamingSchemes);
  for (const std::string& scheme : streaming_schemes_list)
    blink::AddStreamingScheme(scheme.c_str());
  // Parse--Secure-Schemes=scheme1，Scheme2。
  std::vector<std::string> secure_schemes_list =
      ParseSchemesCLISwitch(command_line, switches::kSecureSchemes);
  for (const std::string& scheme : secure_schemes_list)
    url::AddSecureScheme(scheme.data());
  // 我们依赖唯一的进程主机id，它被通知给。
  // 从内容层通过命令行开关进行呈现器处理，
  // 如果出于某种原因将该开关从内容层移除，
  // 我们应该定义我们自己的。
  DCHECK(command_line->HasSwitch(::switches::kRendererClientId));
  renderer_client_id_ =
      command_line->GetSwitchValueASCII(::switches::kRendererClientId);

  g_renderer_client_base = this;
}

RendererClientBase::~RendererClientBase() {
  g_renderer_client_base = nullptr;
}

// 静电。
RendererClientBase* RendererClientBase::Get() {
  DCHECK(g_renderer_client_base);
  return g_renderer_client_base;
}

void RendererClientBase::BindProcess(v8::Isolate* isolate,
                                     gin_helper::Dictionary* process,
                                     content::RenderFrame* render_frame) {
  auto context_id = base::StringPrintf(
      "%s-%" PRId64, renderer_client_id_.c_str(), ++next_context_id_);

  process->SetReadOnly("isMainFrame", render_frame->IsMainFrame());
  process->SetReadOnly("contextIsolated",
                       render_frame->GetBlinkPreferences().context_isolation);
  process->SetReadOnly("contextId", context_id);
}

void RendererClientBase::RenderThreadStarted() {
  auto* command_line = base::CommandLine::ForCurrentProcess();

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  auto* thread = content::RenderThread::Get();

  extensions_client_.reset(CreateExtensionsClient());
  extensions::ExtensionsClient::Set(extensions_client_.get());

  extensions_renderer_client_ =
      std::make_unique<ElectronExtensionsRendererClient>();
  extensions::ExtensionsRendererClient::Set(extensions_renderer_client_.get());

  thread->AddObserver(extensions_renderer_client_->GetDispatcher());
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  // 启用从Chrome PDF查看器打印。
  pdf_print_client_ = std::make_unique<ChromePDFPrintClient>();
  pdf::PepperPDFHost::SetPrintClient(pdf_print_client_.get());
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  spellcheck_ = std::make_unique<SpellCheck>(this);
#endif

  blink::WebCustomElement::AddEmbedderCustomElementName("webview");
  blink::WebCustomElement::AddEmbedderCustomElementName("browserplugin");

  WTF::String extension_scheme(extensions::kExtensionScheme);
  // 扩展资源类似于HTTP，可以安全地公开给FETCH API。这个。
  // 回迁接口规则与XHR一致。
  blink::SchemeRegistry::RegisterURLSchemeAsSupportingFetchAPI(
      extension_scheme);
  // 扩展资源在作为顶级文档加载时应绕过。
  // 布林克严格的第一方原产地检查。
  blink::SchemeRegistry::RegisterURLSchemeAsFirstPartyWhenTopLevel(
      extension_scheme);
  blink::SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
      extension_scheme);

  std::vector<std::string> fetch_enabled_schemes =
      ParseSchemesCLISwitch(command_line, switches::kFetchSchemes);
  for (const std::string& scheme : fetch_enabled_schemes) {
    blink::WebSecurityPolicy::RegisterURLSchemeAsSupportingFetchAPI(
        blink::WebString::FromASCII(scheme));
  }

  std::vector<std::string> service_worker_schemes =
      ParseSchemesCLISwitch(command_line, switches::kServiceWorkerSchemes);
  for (const std::string& scheme : service_worker_schemes)
    blink::WebSecurityPolicy::RegisterURLSchemeAsAllowingServiceWorkers(
        blink::WebString::FromASCII(scheme));

  std::vector<std::string> csp_bypassing_schemes =
      ParseSchemesCLISwitch(command_line, switches::kBypassCSPSchemes);
  for (const std::string& scheme : csp_bypassing_schemes)
    blink::SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
        WTF::String::FromUTF8(scheme.data(), scheme.length()));

  // 默认情况下允许文件方案处理服务工作者。
  // Fixme(Zcbenz)：这个能移到别处吗？
  blink::WebSecurityPolicy::RegisterURLSchemeAsAllowingServiceWorkers("file");
  blink::SchemeRegistry::RegisterURLSchemeAsSupportingFetchAPI("file");

#if defined(OS_WIN)
  // 在渲染器进程中设置ApplicationUserModelID。
  std::wstring app_id =
      command_line->GetSwitchValueNative(switches::kAppUserModelId);
  if (!app_id.empty()) {
    SetCurrentProcessExplicitAppUserModelID(app_id.c_str());
  }
#endif
}

void RendererClientBase::ExposeInterfacesToBrowser(mojo::BinderMap* binders) {
  // 注意：不要在此方法中直接添加活页夹。相反，请修改。
  // 定义|ExposeElectronRendererInterfacesToBrowser()|以确保。
  // 安全审查覆盖范围。
  ExposeElectronRendererInterfacesToBrowser(this, binders);
}

void RendererClientBase::RenderFrameCreated(
    content::RenderFrame* render_frame) {
#if defined(TOOLKIT_VIEWS)
  new AutofillAgent(render_frame,
                    render_frame->GetAssociatedInterfaceRegistry());
#endif
#if BUILDFLAG(ENABLE_PLUGINS)
  new PepperHelper(render_frame);
#endif
  new ContentSettingsObserver(render_frame);
#if BUILDFLAG(ENABLE_PRINTING)
  new printing::PrintRenderFrameHelper(
      render_frame,
      std::make_unique<electron::PrintRenderFrameHelperDelegate>());
#endif

  // 注意：现在必须创建ElectronApiServiceImpl才能捕获。
  // DidCreateDocumentElement事件。
  new ElectronApiServiceImpl(render_frame, this);

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  auto* dispatcher = extensions_renderer_client_->GetDispatcher();
  // 销毁RenderFrame时，ExtensionFrameHelper会自行销毁。
  new extensions::ExtensionFrameHelper(render_frame, dispatcher);

  dispatcher->OnRenderFrameCreated(render_frame);

  render_frame->GetAssociatedInterfaceRegistry()->AddInterface(
      base::BindRepeating(
          &extensions::MimeHandlerViewContainerManager::BindReceiver,
          render_frame->GetRoutingID()));
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  if (render_frame->GetBlinkPreferences().enable_spellcheck)
    new SpellCheckProvider(render_frame, spellcheck_.get(), this);
#endif
}

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
void RendererClientBase::GetInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  // TODO(crbug.com/977637)：取消使用。
  // |service_manager：：LocalInterfaceProvider|。这样做只是为了避免。
  // 搅动拼写检查代码，同时消除“Chrome”和。
  // “Chrome_renender”服务。拼写检查是(也应该继续)唯一。
  // 此实现的使用者。
  content::RenderThread::Get()->BindHostReceiver(
      mojo::GenericPendingReceiver(interface_name, std::move(interface_pipe)));
}
#endif

void RendererClientBase::DidClearWindowObject(
    content::RenderFrame* render_frame) {
  // 确保每个页面都会创建一个脚本上下文。
  render_frame->GetWebFrame()->ExecuteScript(blink::WebScriptSource("void 0"));
}

bool RendererClientBase::OverrideCreatePlugin(
    content::RenderFrame* render_frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {
  if (params.mime_type.Utf8() == content::kBrowserPluginMimeType ||
#if BUILDFLAG(ENABLE_PDF_VIEWER)
      params.mime_type.Utf8() == kPdfPluginMimeType ||
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)。
      render_frame->GetBlinkPreferences().enable_plugins)
    return false;

  *plugin = nullptr;
  return true;
}

void RendererClientBase::AddSupportedKeySystems(
    std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems) {
#if defined(WIDEVINE_CDM_AVAILABLE)
  key_systems_provider_.AddSupportedKeySystems(key_systems);
#endif
}

bool RendererClientBase::IsKeySystemsUpdateNeeded() {
#if defined(WIDEVINE_CDM_AVAILABLE)
  return key_systems_provider_.IsKeySystemsUpdateNeeded();
#else
  return false;
#endif
}

void RendererClientBase::DidSetUserAgent(const std::string& user_agent) {
#if BUILDFLAG(ENABLE_PRINTING)
  printing::SetAgent(user_agent);
#endif
}

bool RendererClientBase::IsPluginHandledExternally(
    content::RenderFrame* render_frame,
    const blink::WebElement& plugin_element,
    const GURL& original_url,
    const std::string& mime_type) {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  DCHECK(plugin_element.HasHTMLTagName("object") ||
         plugin_element.HasHTMLTagName("embed"));
  // TODO(Nornagon)：此信息应与中的数据共享。
  // Electronics_Content_client.cc/ComputeBuiltInPlugins。
  content::WebPluginInfo info;
  info.type = content::WebPluginInfo::PLUGIN_TYPE_BROWSER_PLUGIN;
  const char16_t kPDFExtensionPluginName[] = u"Chromium PDF Viewer";
  info.name = kPDFExtensionPluginName;
  info.path = base::FilePath::FromUTF8Unsafe(extension_misc::kPdfExtensionId);
  info.background_color = content::WebPluginInfo::kDefaultBackgroundColor;
  info.mime_types.emplace_back("application/pdf", "pdf",
                               "Portable Document Format");
  return extensions::MimeHandlerViewContainerManager::Get(
             content::RenderFrame::FromWebFrame(
                 plugin_element.GetDocument().GetFrame()),
             true /* CREATE_IF_DOS_NOT_EXIST。*/)
      ->CreateFrameContainer(plugin_element, original_url, mime_type, info);
#else
  return false;
#endif
}

bool RendererClientBase::IsOriginIsolatedPepperPlugin(
    const base::FilePath& plugin_path) {
  // 隔离所有Pepper插件，包括PDF插件。
  return true;
}

std::unique_ptr<blink::WebPrescientNetworking>
RendererClientBase::CreatePrescientNetworking(
    content::RenderFrame* render_frame) {
  return std::make_unique<network_hints::WebPrescientNetworkingImpl>(
      render_frame);
}

void RendererClientBase::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_renderer_client_.get()->RunScriptsAtDocumentStart(render_frame);
#endif
}

void RendererClientBase::RunScriptsAtDocumentIdle(
    content::RenderFrame* render_frame) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_renderer_client_.get()->RunScriptsAtDocumentIdle(render_frame);
#endif
}

void RendererClientBase::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_renderer_client_.get()->RunScriptsAtDocumentEnd(render_frame);
#endif
}

bool RendererClientBase::AllowScriptExtensionForServiceWorker(
    const url::Origin& script_origin) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  return script_origin.scheme() == extensions::kExtensionScheme;
#else
  return false;
#endif
}

void RendererClientBase::DidInitializeServiceWorkerContextOnWorkerThread(
    blink::WebServiceWorkerContextProxy* context_proxy,
    const GURL& service_worker_scope,
    const GURL& script_url) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_renderer_client_->GetDispatcher()
      ->DidInitializeServiceWorkerContextOnWorkerThread(
          context_proxy, service_worker_scope, script_url);
#endif
}

void RendererClientBase::WillEvaluateServiceWorkerOnWorkerThread(
    blink::WebServiceWorkerContextProxy* context_proxy,
    v8::Local<v8::Context> v8_context,
    int64_t service_worker_version_id,
    const GURL& service_worker_scope,
    const GURL& script_url) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_renderer_client_->GetDispatcher()
      ->WillEvaluateServiceWorkerOnWorkerThread(
          context_proxy, v8_context, service_worker_version_id,
          service_worker_scope, script_url);
#endif
}

void RendererClientBase::DidStartServiceWorkerContextOnWorkerThread(
    int64_t service_worker_version_id,
    const GURL& service_worker_scope,
    const GURL& script_url) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_renderer_client_->GetDispatcher()
      ->DidStartServiceWorkerContextOnWorkerThread(
          service_worker_version_id, service_worker_scope, script_url);
#endif
}

void RendererClientBase::WillDestroyServiceWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context,
    int64_t service_worker_version_id,
    const GURL& service_worker_scope,
    const GURL& script_url) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_renderer_client_->GetDispatcher()
      ->WillDestroyServiceWorkerContextOnWorkerThread(
          context, service_worker_version_id, service_worker_scope, script_url);
#endif
}

v8::Local<v8::Context> RendererClientBase::GetContext(
    blink::WebLocalFrame* frame,
    v8::Isolate* isolate) const {
  auto* render_frame = content::RenderFrame::FromWebFrame(frame);
  DCHECK(render_frame);
  if (render_frame && render_frame->GetBlinkPreferences().context_isolation)
    return frame->GetScriptContextFromWorldId(isolate,
                                              WorldIDs::ISOLATED_WORLD_ID);
  else
    return frame->MainWorldScriptContext();
}

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
extensions::ExtensionsClient* RendererClientBase::CreateExtensionsClient() {
  return new ElectronExtensionsClient;
}
#endif

bool RendererClientBase::IsWebViewFrame(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) const {
  auto* isolate = context->GetIsolate();

  if (render_frame->IsMainFrame())
    return false;

  gin::Dictionary window_dict(
      isolate, GetContext(render_frame->GetWebFrame(), isolate)->Global());

  v8::Local<v8::Object> frame_element;
  if (!window_dict.Get("frameElement", &frame_element))
    return false;

  gin_helper::Dictionary frame_element_dict(isolate, frame_element);

  bool is_webview = false;
  return frame_element_dict.GetHidden("isWebView", &is_webview) && is_webview;
}

void RendererClientBase::SetupMainWorldOverrides(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  auto prefs = render_frame->GetBlinkPreferences();
  // 如果启用了WebView，我们只需要运行隔离捆绑包。
  if (!prefs.webview_tag)
    return;

  // 主世界上下文中的设置窗口覆盖。
  // 将捆绑包包装到一个函数中，该函数将solatedApi接收为。
  // 一场争论。
  auto* isolate = context->GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);

  gin_helper::Dictionary isolated_api = gin::Dictionary::CreateEmpty(isolate);
  isolated_api.SetMethod("allowGuestViewElementDefinition",
                         &AllowGuestViewElementDefinition);
  isolated_api.SetMethod("setIsWebView", &SetIsWebView);

  auto source_context = GetContext(render_frame->GetWebFrame(), isolate);
  gin_helper::Dictionary global(isolate, source_context->Global());

  v8::Local<v8::Value> guest_view_internal;
  if (global.GetHidden("guestViewInternal", &guest_view_internal)) {
    api::context_bridge::ObjectCache object_cache;
    auto result = api::PassValueToOtherContext(
        source_context, context, guest_view_internal, &object_cache, false, 0);
    if (!result.IsEmpty()) {
      isolated_api.Set("guestViewInternal", result.ToLocalChecked());
    }
  }

  std::vector<v8::Local<v8::String>> isolated_bundle_params = {
      node::FIXED_ONE_BYTE_STRING(isolate, "isolatedApi")};

  std::vector<v8::Local<v8::Value>> isolated_bundle_args = {
      isolated_api.GetHandle()};

  util::CompileAndCall(context, "electron/js2c/isolated_bundle",
                       &isolated_bundle_params, &isolated_bundle_args, nullptr);
}

// 静电。
void RendererClientBase::AllowGuestViewElementDefinition(
    v8::Isolate* isolate,
    v8::Local<v8::Object> context,
    v8::Local<v8::Function> register_cb) {
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context->CreationContext());
  blink::WebCustomElement::EmbedderNamesAllowedScope embedder_names_scope;

  content::RenderFrame* render_frame = GetRenderFrame(context);
  if (!render_frame)
    return;

  render_frame->GetWebFrame()->RequestExecuteV8Function(
      context->CreationContext(), register_cb, v8::Null(isolate), 0, nullptr,
      nullptr);
}

}  // 命名空间电子
