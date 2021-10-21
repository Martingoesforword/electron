// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/api/tabs/tabs_api.h"

#include <memory>
#include <utility>

#include "chrome/common/url_constants.h"
#include "components/url_formatter/url_fixer.h"
#include "content/public/browser/navigation_entry.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/mojom/host_id.mojom.h"
#include "extensions/common/permissions/permissions_data.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/web_contents_zoom_controller.h"
#include "shell/common/extensions/api/tabs.h"
#include "third_party/blink/public/common/page/page_zoom.h"
#include "url/gurl.h"

using electron::WebContentsZoomController;

namespace extensions {

namespace tabs = api::tabs;

const char kFrameNotFoundError[] = "No frame with id * in tab *.";
const char kPerOriginOnlyInAutomaticError[] =
    "Can only set scope to "
    "\"per-origin\" in \"automatic\" mode.";

using api::extension_types::InjectDetails;

namespace {
void ZoomModeToZoomSettings(WebContentsZoomController::ZoomMode zoom_mode,
                            api::tabs::ZoomSettings* zoom_settings) {
  DCHECK(zoom_settings);
  switch (zoom_mode) {
    case WebContentsZoomController::ZoomMode::kDefault:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_AUTOMATIC;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_ORIGIN;
      break;
    case WebContentsZoomController::ZoomMode::kIsolated:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_AUTOMATIC;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
      break;
    case WebContentsZoomController::ZoomMode::kManual:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_MANUAL;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
      break;
    case WebContentsZoomController::ZoomMode::kDisabled:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_DISABLED;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
      break;
  }
}
}  // 命名空间。

ExecuteCodeInTabFunction::ExecuteCodeInTabFunction() : execute_tab_id_(-1) {}

ExecuteCodeInTabFunction::~ExecuteCodeInTabFunction() = default;

ExecuteCodeFunction::InitResult ExecuteCodeInTabFunction::Init() {
  if (init_result_)
    return init_result_.value();

  if (args().size() < 2)
    return set_init_result(VALIDATION_FAILURE);

  const auto& tab_id_value = args()[0];
  // |tab_id|可选，如果没有也没关系。
  int tab_id = -1;
  if (tab_id_value.is_int()) {
    // 但如果它是存在的，它就需要是非负面的。
    tab_id = tab_id_value.GetInt();
    if (tab_id < 0) {
      return set_init_result(VALIDATION_FAILURE);
    }
  }

  // |详情|非可选。
  const base::Value& details_value = args()[1];
  if (!details_value.is_dict())
    return set_init_result(VALIDATION_FAILURE);
  std::unique_ptr<InjectDetails> details(new InjectDetails());
  if (!InjectDetails::Populate(details_value, details.get()))
    return set_init_result(VALIDATION_FAILURE);

  if (tab_id == -1) {
    // 在Electron中没有有用的“默认标签”概念。
    // TODO(Nornagon)：我们可能会把它踢到一个事件上，以允许。
    // 应用程序决定“默认选项卡”对他们意味着什么？
    return set_init_result(VALIDATION_FAILURE);
  }

  execute_tab_id_ = tab_id;
  details_ = std::move(details);
  set_host_id(
      mojom::HostID(mojom::HostID::HostType::kExtensions, extension()->id()));
  return set_init_result(SUCCESS);
}

bool ExecuteCodeInTabFunction::CanExecuteScriptOnPage(std::string* error) {
  // 如果指定了|tab_id|，请查找该选项卡。否则默认为选定。
  // 当前窗口中的选项卡。
  CHECK_GE(execute_tab_id_, 0);
  auto* contents = electron::api::WebContents::FromID(execute_tab_id_);
  if (!contents) {
    return false;
  }

  int frame_id = details_->frame_id ? *details_->frame_id
                                    : ExtensionApiFrameIdMap::kTopFrameId;
  content::RenderFrameHost* rfh =
      ExtensionApiFrameIdMap::GetRenderFrameHostById(contents->web_contents(),
                                                     frame_id);
  if (!rfh) {
    *error = ErrorUtils::FormatErrorMessage(
        kFrameNotFoundError, base::NumberToString(frame_id),
        base::NumberToString(execute_tab_id_));
    return false;
  }

  // 清单.json中声明的内容脚本可以访问大约：-urls处的框架。
  // 如果扩展名有权访问帧的原点，则还允许。
  // 编程内容脚本位于以下位置：-允许来源的URL。
  GURL effective_document_url(rfh->GetLastCommittedURL());
  bool is_about_url = effective_document_url.SchemeIs(url::kAboutScheme);
  if (is_about_url && details_->match_about_blank &&
      *details_->match_about_blank) {
    effective_document_url = GURL(rfh->GetLastCommittedOrigin().Serialize());
  }

  if (!effective_document_url.is_valid()) {
    // 未知URL，例如因为尚未提交加载。考虑到现在，
    // 渲染器将再次检查，如果需要，注入将失败。
    return true;
  }

  // 注意：由于竞争条件，这可能会给出错误的答案，但这是可以的。
  // 我们再次签入渲染器。
  if (!extension()->permissions_data()->CanAccessPage(effective_document_url,
                                                      execute_tab_id_, error)) {
    if (is_about_url &&
        extension()->permissions_data()->active_permissions().HasAPIPermission(
            extensions::mojom::APIPermissionID::kTab)) {
      *error = ErrorUtils::FormatErrorMessage(
          manifest_errors::kCannotAccessAboutUrl,
          rfh->GetLastCommittedURL().spec(),
          rfh->GetLastCommittedOrigin().Serialize());
    }
    return false;
  }

  return true;
}

ScriptExecutor* ExecuteCodeInTabFunction::GetScriptExecutor(
    std::string* error) {
  auto* contents = electron::api::WebContents::FromID(execute_tab_id_);
  if (!contents)
    return nullptr;
  return contents->script_executor();
}

bool ExecuteCodeInTabFunction::IsWebView() const {
  return false;
}

const GURL& ExecuteCodeInTabFunction::GetWebViewSrc() const {
  return GURL::EmptyGURL();
}

bool TabsExecuteScriptFunction::ShouldInsertCSS() const {
  return false;
}

bool TabsExecuteScriptFunction::ShouldRemoveCSS() const {
  return false;
}

ExtensionFunction::ResponseAction TabsGetFunction::Run() {
  std::unique_ptr<tabs::Get::Params> params(tabs::Get::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  int tab_id = params->tab_id;

  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  tabs::Tab tab;

  tab.id = std::make_unique<int>(tab_id);
  // TODO(Nornagon)：在Chrome中，选项卡URL仅对扩展模块可用。
  // 具有“选项卡”(或“activeTab”)权限的。我们也应该这么做。
  // 许可检查在这里。
  tab.url = std::make_unique<std::string>(
      contents->web_contents()->GetLastCommittedURL().spec());

  tab.active = contents->IsFocused();

  return RespondNow(ArgumentList(tabs::Get::Results::Create(std::move(tab))));
}

ExtensionFunction::ResponseAction TabsSetZoomFunction::Run() {
  std::unique_ptr<tabs::SetZoom::Params> params(
      tabs::SetZoom::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  auto* web_contents = contents->web_contents();
  GURL url(web_contents->GetVisibleURL());
  std::string error;
  if (extension()->permissions_data()->IsRestrictedUrl(url, &error))
    return RespondNow(Error(error));

  auto* zoom_controller = contents->GetZoomController();
  double zoom_level =
      params->zoom_factor > 0
          ? blink::PageZoomFactorToZoomLevel(params->zoom_factor)
          : blink::PageZoomFactorToZoomLevel(
                zoom_controller->GetDefaultZoomFactor());

  zoom_controller->SetZoomLevel(zoom_level);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction TabsGetZoomFunction::Run() {
  std::unique_ptr<tabs::GetZoom::Params> params(
      tabs::GetZoom::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  double zoom_level = contents->GetZoomController()->GetZoomLevel();
  double zoom_factor = blink::PageZoomLevelToZoomFactor(zoom_level);

  return RespondNow(ArgumentList(tabs::GetZoom::Results::Create(zoom_factor)));
}

ExtensionFunction::ResponseAction TabsGetZoomSettingsFunction::Run() {
  std::unique_ptr<tabs::GetZoomSettings::Params> params(
      tabs::GetZoomSettings::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  auto* zoom_controller = contents->GetZoomController();
  WebContentsZoomController::ZoomMode zoom_mode =
      contents->GetZoomController()->zoom_mode();
  api::tabs::ZoomSettings zoom_settings;
  ZoomModeToZoomSettings(zoom_mode, &zoom_settings);
  zoom_settings.default_zoom_factor = std::make_unique<double>(
      blink::PageZoomLevelToZoomFactor(zoom_controller->GetDefaultZoomLevel()));

  return RespondNow(
      ArgumentList(api::tabs::GetZoomSettings::Results::Create(zoom_settings)));
}

ExtensionFunction::ResponseAction TabsSetZoomSettingsFunction::Run() {
  using api::tabs::ZoomSettings;

  std::unique_ptr<tabs::SetZoomSettings::Params> params(
      tabs::SetZoomSettings::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  std::string error;
  GURL url(contents->web_contents()->GetVisibleURL());
  if (extension()->permissions_data()->IsRestrictedUrl(url, &error))
    return RespondNow(Error(error));

  // “Per-Origin”作用域仅在“Automatic”模式下可用。
  if (params->zoom_settings.scope == tabs::ZOOM_SETTINGS_SCOPE_PER_ORIGIN &&
      params->zoom_settings.mode != tabs::ZOOM_SETTINGS_MODE_AUTOMATIC &&
      params->zoom_settings.mode != tabs::ZOOM_SETTINGS_MODE_NONE) {
    return RespondNow(Error(kPerOriginOnlyInAutomaticError));
  }

  // 确定要从|web_content|设置为的正确内部缩放模式。
  // 用户指定|ZOOM_SETTINGS|。
  WebContentsZoomController::ZoomMode zoom_mode =
      WebContentsZoomController::ZoomMode::kDefault;
  switch (params->zoom_settings.mode) {
    case tabs::ZOOM_SETTINGS_MODE_NONE:
    case tabs::ZOOM_SETTINGS_MODE_AUTOMATIC:
      switch (params->zoom_settings.scope) {
        case tabs::ZOOM_SETTINGS_SCOPE_NONE:
        case tabs::ZOOM_SETTINGS_SCOPE_PER_ORIGIN:
          zoom_mode = WebContentsZoomController::ZoomMode::kDefault;
          break;
        case tabs::ZOOM_SETTINGS_SCOPE_PER_TAB:
          zoom_mode = WebContentsZoomController::ZoomMode::kIsolated;
      }
      break;
    case tabs::ZOOM_SETTINGS_MODE_MANUAL:
      zoom_mode = WebContentsZoomController::ZoomMode::kManual;
      break;
    case tabs::ZOOM_SETTINGS_MODE_DISABLED:
      zoom_mode = WebContentsZoomController::ZoomMode::kDisabled;
  }

  contents->GetZoomController()->SetZoomMode(zoom_mode);

  return RespondNow(NoArguments());
}

bool IsKillURL(const GURL& url) {
#if DCHECK_IS_ON()
  // 调用方应确保|url|已由。
  // Url_Formatter：：FixupURL，它(在许多事情中)负责。
  // 重写的内容：kill to Chrome：//kill/。
  if (url.SchemeIs(url::kAboutScheme))
    DCHECK(url.IsAboutBlank() || url.IsAboutSrcdoc());
#endif

  static const char* const kill_hosts[] = {
      chrome::kChromeUICrashHost,         chrome::kChromeUIDelayedHangUIHost,
      chrome::kChromeUIHangUIHost,        chrome::kChromeUIKillHost,
      chrome::kChromeUIQuitHost,          chrome::kChromeUIRestartHost,
      content::kChromeUIBrowserCrashHost, content::kChromeUIMemoryExhaustHost,
  };

  if (!url.SchemeIs(content::kChromeUIScheme))
    return false;

  return base::Contains(kill_hosts, url.host_piece());
}

GURL ResolvePossiblyRelativeURL(const std::string& url_string,
                                const Extension* extension) {
  GURL url = GURL(url_string);
  if (!url.is_valid() && extension)
    url = extension->GetResourceURL(url_string);

  return url;
}
bool PrepareURLForNavigation(const std::string& url_string,
                             const Extension* extension,
                             GURL* return_url,
                             std::string* error) {
  GURL url = ResolvePossiblyRelativeURL(url_string, extension);

  // 理想情况下，URL只对用户输入(例如URL)是“固定的”
  // 进入Omnibox)，但一些扩展依赖于遗留行为。
  // 在那里所有的航行都受到“修理”的影响。另请参阅。
  // Https://crbug.com/1145381.。
  url = url_formatter::FixupURL(url.spec(), "" /* =所需的_TLD。*/);

  // 拒绝无效的URL。
  if (!url.is_valid()) {
    const char kInvalidUrlError[] = "Invalid url: \"*\".";
    *error = ErrorUtils::FormatErrorMessage(kInvalidUrlError, url_string);
    return false;
  }

  // 不要让扩展使浏览器或渲染器崩溃。
  if (IsKillURL(url)) {
    const char kNoCrashBrowserError[] =
        "I'm sorry. I'm afraid I can't do that.";
    *error = kNoCrashBrowserError;
    return false;
  }

  // 不要让扩展直接导航到DevTools方案页面，除非。
  // 他们拥有适用的权限。
  if (url.SchemeIs(content::kChromeDevToolsScheme) &&
      !(extension->permissions_data()->HasAPIPermission(
            extensions::mojom::APIPermissionID::kDevtools) ||
        extension->permissions_data()->HasAPIPermission(
            extensions::mojom::APIPermissionID::kDebugger))) {
    const char kCannotNavigateToDevtools[] =
        "Cannot navigate to a devtools:// 没有DevTools或“。
        "debugger permission.";
    *error = kCannotNavigateToDevtools;
    return false;
  }

  return_url->Swap(&url);
  return true;
}

TabsUpdateFunction::TabsUpdateFunction() : web_contents_(nullptr) {}

ExtensionFunction::ResponseAction TabsUpdateFunction::Run() {
  std::unique_ptr<tabs::Update::Params> params(
      tabs::Update::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  web_contents_ = contents->web_contents();

  // 如果URL不同，请将选项卡导航到新位置。
  std::string error;
  if (params->update_properties.url.get()) {
    std::string updated_url = *params->update_properties.url;
    if (!UpdateURL(updated_url, tab_id, &error))
      return RespondNow(Error(std::move(error)));
  }

  if (params->update_properties.muted.get()) {
    contents->SetAudioMuted(*params->update_properties.muted);
  }

  return RespondNow(GetResult());
}

bool TabsUpdateFunction::UpdateURL(const std::string& url_string,
                                   int tab_id,
                                   std::string* error) {
  GURL url;
  if (!PrepareURLForNavigation(url_string, extension(), &url, error)) {
    return false;
  }

  const bool is_javascript_scheme = url.SchemeIs(url::kJavaScriptScheme);
  // Chrome.tab.update()中禁止使用JavaScript URL。
  if (is_javascript_scheme) {
    const char kJavaScriptUrlsNotAllowedInTabsUpdate[] =
        "JavaScript URLs are not allowed in chrome.tabs.update. Use "
        "chrome.tabs.executeScript instead.";
    *error = kJavaScriptUrlsNotAllowedInTabsUpdate;
    return false;
  }

  content::NavigationController::LoadURLParams load_params(url);

  // 将扩展模块启动的导航视为渲染器启动，以便URL。
  // 在提交之前不会显示在全能框中。这避免了URL欺骗。
  // 因为URL可以代表不可信的内容打开。
  load_params.is_renderer_initiated = true;
  // 所有渲染器启动的导航都需要具有启动器原点。
  load_params.initiator_origin = extension()->origin();
  // |SOURCE_SITE_INSTANCE|需要设置渲染器进程。
  // 站点隔离选择与|Initiator_Origin|兼容。
  load_params.source_site_instance = content::SiteInstance::CreateForURL(
      web_contents_->GetBrowserContext(),
      load_params.initiator_origin->GetURL());

  // 将导航标记为通过API启动意味着焦点。
  // 将留在无所不包的盒子中-参见https://crbug.com/1085779.。
  load_params.transition_type = ui::PAGE_TRANSITION_FROM_API;

  web_contents_->GetController().LoadURLWithParams(load_params);

  DCHECK_EQ(url,
            web_contents_->GetController().GetPendingEntry()->GetVirtualURL());

  return true;
}

ExtensionFunction::ResponseValue TabsUpdateFunction::GetResult() {
  if (!has_callback())
    return NoArguments();

  tabs::Tab tab;

  auto* api_web_contents = electron::api::WebContents::From(web_contents_);
  tab.id =
      std::make_unique<int>(api_web_contents ? api_web_contents->ID() : -1);
  // TODO(Nornagon)：在Chrome中，选项卡URL仅对扩展模块可用。
  // 具有“选项卡”(或“activeTab”)权限的。我们也应该这么做。
  // 许可检查在这里。
  tab.url = std::make_unique<std::string>(
      web_contents_->GetLastCommittedURL().spec());

  return ArgumentList(tabs::Get::Results::Create(std::move(tab)));
}

void TabsUpdateFunction::OnExecuteCodeFinished(
    const std::string& error,
    const GURL& url,
    const base::ListValue& script_result) {
  if (!error.empty()) {
    Respond(Error(error));
    return;
  }

  return Respond(GetResult());
}

}  // 命名空间扩展
