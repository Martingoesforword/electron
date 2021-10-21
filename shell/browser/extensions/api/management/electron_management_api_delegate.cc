// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

// TODO(Sentialx)：在Electron的会话中发出相关事件？
#include "shell/browser/extensions/api/management/electron_management_api_delegate.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "chrome/common/extensions/extension_metrics.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/api/management/management_api.h"
#include "extensions/browser/api/management/management_api_constants.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/api/management.h"
#include "extensions/common/extension.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "third_party/blink/public/mojom/manifest/display_mode.mojom.h"

namespace {
class ManagementSetEnabledFunctionInstallPromptDelegate
    : public extensions::InstallPromptDelegate {
 public:
  ManagementSetEnabledFunctionInstallPromptDelegate(
      content::WebContents* web_contents,
      content::BrowserContext* browser_context,
      const extensions::Extension* extension,
      base::OnceCallback<void(bool)> callback) {
    // TODO(Sentialx)：发射事件。
  }
  ~ManagementSetEnabledFunctionInstallPromptDelegate() override = default;

 private:
  base::WeakPtrFactory<ManagementSetEnabledFunctionInstallPromptDelegate>
      weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ManagementSetEnabledFunctionInstallPromptDelegate);
};

class ManagementUninstallFunctionUninstallDialogDelegate
    : public extensions::UninstallDialogDelegate {
 public:
  ManagementUninstallFunctionUninstallDialogDelegate(
      extensions::ManagementUninstallFunctionBase* function,
      const extensions::Extension* target_extension,
      bool show_programmatic_uninstall_ui) {
    // TODO(Sentialx)：发射事件。
  }

  ~ManagementUninstallFunctionUninstallDialogDelegate() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(ManagementUninstallFunctionUninstallDialogDelegate);
};

}  // 命名空间。

ElectronManagementAPIDelegate::ElectronManagementAPIDelegate() = default;

ElectronManagementAPIDelegate::~ElectronManagementAPIDelegate() = default;

void ElectronManagementAPIDelegate::LaunchAppFunctionDelegate(
    const extensions::Extension* extension,
    content::BrowserContext* context) const {
  // TODO(Sentialx)：发射事件。
  extensions::RecordAppLaunchType(extension_misc::APP_LAUNCH_EXTENSION_API,
                                  extension->GetType());
}

GURL ElectronManagementAPIDelegate::GetFullLaunchURL(
    const extensions::Extension* extension) const {
  return extensions::AppLaunchInfo::GetFullLaunchURL(extension);
}

extensions::LaunchType ElectronManagementAPIDelegate::GetLaunchType(
    const extensions::ExtensionPrefs* prefs,
    const extensions::Extension* extension) const {
  // 待办事项(伤感)。
  return extensions::LAUNCH_TYPE_DEFAULT;
}

void ElectronManagementAPIDelegate::
    GetPermissionWarningsByManifestFunctionDelegate(
        extensions::ManagementGetPermissionWarningsByManifestFunction* function,
        const std::string& manifest_str) const {
  data_decoder::DataDecoder::ParseJsonIsolated(
      manifest_str,
      base::BindOnce(
          &extensions::ManagementGetPermissionWarningsByManifestFunction::
              OnParse,
          function));
}

std::unique_ptr<extensions::InstallPromptDelegate>
ElectronManagementAPIDelegate::SetEnabledFunctionDelegate(
    content::WebContents* web_contents,
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    base::OnceCallback<void(bool)> callback) const {
  return std::make_unique<ManagementSetEnabledFunctionInstallPromptDelegate>(
      web_contents, browser_context, extension, std::move(callback));
}

std::unique_ptr<extensions::UninstallDialogDelegate>
ElectronManagementAPIDelegate::UninstallFunctionDelegate(
    extensions::ManagementUninstallFunctionBase* function,
    const extensions::Extension* target_extension,
    bool show_programmatic_uninstall_ui) const {
  return std::make_unique<ManagementUninstallFunctionUninstallDialogDelegate>(
      function, target_extension, show_programmatic_uninstall_ui);
}

bool ElectronManagementAPIDelegate::CreateAppShortcutFunctionDelegate(
    extensions::ManagementCreateAppShortcutFunction* function,
    const extensions::Extension* extension,
    std::string* error) const {
  return false;  // TODO(Sentialx)：路由事件并返回true。
}

std::unique_ptr<extensions::AppForLinkDelegate>
ElectronManagementAPIDelegate::GenerateAppForLinkFunctionDelegate(
    extensions::ManagementGenerateAppForLinkFunction* function,
    content::BrowserContext* context,
    const std::string& title,
    const GURL& launch_url) const {
  // 待办事项(伤感)。
  return nullptr;
}

bool ElectronManagementAPIDelegate::CanContextInstallWebApps(
    content::BrowserContext* context) const {
  // 待办事项(伤感)。
  return false;
}

void ElectronManagementAPIDelegate::InstallOrLaunchReplacementWebApp(
    content::BrowserContext* context,
    const GURL& web_app_url,
    InstallOrLaunchWebAppCallback callback) const {
  // 待办事项(伤感)。
}

bool ElectronManagementAPIDelegate::CanContextInstallAndroidApps(
    content::BrowserContext* context) const {
  return false;
}

void ElectronManagementAPIDelegate::CheckAndroidAppInstallStatus(
    const std::string& package_name,
    AndroidAppInstallStatusCallback callback) const {
  std::move(callback).Run(false);
}

void ElectronManagementAPIDelegate::InstallReplacementAndroidApp(
    const std::string& package_name,
    InstallAndroidAppCallback callback) const {
  std::move(callback).Run(false);
}

void ElectronManagementAPIDelegate::EnableExtension(
    content::BrowserContext* context,
    const std::string& extension_id) const {
  // Const Extensions：：Extension*Extension=。
  // Extensions：：ExtensionRegistry：：Get(context)-&gt;GetExtensionById(。
  // Extension_id，Extensions：：ExtensionRegistry：：Everything)；

  // TODO(Sentialx)：我们没有扩展服务。
  // 如果扩展因权限增加而被禁用，则管理。
  // API将向用户显示重新启用提示，因此我们知道。
  // 在此授予权限是安全的。
  // Extensions：：ExtensionSystem：：Get(Context)。
  // -&gt;EXTENSION_SERVICE()。
  // -&gt;GrantPermissionsAndEnableExtension(extension)；
}

void ElectronManagementAPIDelegate::DisableExtension(
    content::BrowserContext* context,
    const extensions::Extension* source_extension,
    const std::string& extension_id,
    extensions::disable_reason::DisableReason disable_reason) const {
  // TODO(Sentialx)：我们没有扩展服务。
  // Extensions：：ExtensionSystem：：Get(Context)。
  // -&gt;EXTENSION_SERVICE()。
  // -&gt;DisableExtensionWithSource(source_extension，扩展ID，
  // DISABLE_REASON)；
}

bool ElectronManagementAPIDelegate::UninstallExtension(
    content::BrowserContext* context,
    const std::string& transient_extension_id,
    extensions::UninstallReason reason,
    std::u16string* error) const {
  // TODO(Sentialx)：我们没有扩展服务。
  // 返回Extensions：：ExtensionSystem：：Get(Context)。
  // -&gt;EXTENSION_SERVICE()。
  // -&gt;UninstallExtension(瞬态_扩展_id，原因，错误)；
  return false;
}

void ElectronManagementAPIDelegate::SetLaunchType(
    content::BrowserContext* context,
    const std::string& extension_id,
    extensions::LaunchType launch_type) const {
  // 待办事项(伤感)。
  // Extensions：：SetLaunchType(Context，Extension_id，Launch_type)；
}

GURL ElectronManagementAPIDelegate::GetIconURL(
    const extensions::Extension* extension,
    int icon_size,
    ExtensionIconSet::MatchType match,
    bool grayscale) const {
  GURL icon_url(base::StringPrintf("%s%s/%d/%d%s",
                                   chrome::kChromeUIExtensionIconURL,
                                   extension->id().c_str(), icon_size, match,
                                   grayscale ? "?grayscale=true" : ""));
  CHECK(icon_url.is_valid());
  return icon_url;
}

GURL ElectronManagementAPIDelegate::GetEffectiveUpdateURL(
    const extensions::Extension& extension,
    content::BrowserContext* context) const {
  // TODO(Codebytere)：我们当前不支持ExtensionManagement。
  return GURL::EmptyGURL();
}
