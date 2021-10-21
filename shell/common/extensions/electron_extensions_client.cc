// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/common/extensions/electron_extensions_client.h"

#include <memory>
#include <string>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "components/version_info/version_info.h"
#include "content/public/common/user_agent.h"
#include "extensions/common/core_extensions_api_provider.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/features/simple_feature.h"
#include "extensions/common/permissions/permission_message_provider.h"
#include "extensions/common/url_pattern_set.h"
#include "shell/common/extensions/electron_extensions_api_provider.h"

using extensions::ExtensionsClient;

namespace electron {

namespace {

// TODO(JamesCook)：重构ChromePermissionsMessageProvider，以便我们可以共享。
// 密码。目前，此实现什么也不做。
class ElectronPermissionMessageProvider
    : public extensions::PermissionMessageProvider {
 public:
  ElectronPermissionMessageProvider() = default;
  ~ElectronPermissionMessageProvider() override = default;

  // PermissionMessageProvider实现。
  extensions::PermissionMessages GetPermissionMessages(
      const extensions::PermissionIDSet& permissions) const override {
    return extensions::PermissionMessages();
  }

  bool IsPrivilegeIncrease(
      const extensions::PermissionSet& granted_permissions,
      const extensions::PermissionSet& requested_permissions,
      extensions::Manifest::Type extension_type) const override {
    // 确保我们在发货前实施这一点。
    CHECK(false);
    return false;
  }

  extensions::PermissionIDSet GetAllPermissionIDs(
      const extensions::PermissionSet& permissions,
      extensions::Manifest::Type extension_type) const override {
    return extensions::PermissionIDSet();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronPermissionMessageProvider);
};

base::LazyInstance<ElectronPermissionMessageProvider>::DestructorAtExit
    g_permission_message_provider = LAZY_INSTANCE_INITIALIZER;

}  // 命名空间。

ElectronExtensionsClient::ElectronExtensionsClient()
    : webstore_base_url_(extension_urls::kChromeWebstoreBaseURL),
      webstore_update_url_(extension_urls::kChromeWebstoreUpdateURL) {
  AddAPIProvider(std::make_unique<extensions::CoreExtensionsAPIProvider>());
  AddAPIProvider(std::make_unique<ElectronExtensionsAPIProvider>());
}

ElectronExtensionsClient::~ElectronExtensionsClient() = default;

void ElectronExtensionsClient::Initialize() {
  // TODO(詹姆斯库克)：我们需要把任何分机列入白名单吗？
}

void ElectronExtensionsClient::InitializeWebStoreUrls(
    base::CommandLine* command_line) {}

const extensions::PermissionMessageProvider&
ElectronExtensionsClient::GetPermissionMessageProvider() const {
  NOTIMPLEMENTED();
  return g_permission_message_provider.Get();
}

const std::string ElectronExtensionsClient::GetProductName() {
  // TODO(Samuelmaddock)：
  return "app_shell";
}

void ElectronExtensionsClient::FilterHostPermissions(
    const extensions::URLPatternSet& hosts,
    extensions::URLPatternSet* new_hosts,
    extensions::PermissionIDSet* permissions) const {
  NOTIMPLEMENTED();
}

void ElectronExtensionsClient::SetScriptingAllowlist(
    const ExtensionsClient::ScriptingAllowlist& allowlist) {
  scripting_allowlist_ = allowlist;
}

const ExtensionsClient::ScriptingAllowlist&
ElectronExtensionsClient::GetScriptingAllowlist() const {
  // 托多(詹姆斯库克)：真正的白名单。
  return scripting_allowlist_;
}

extensions::URLPatternSet
ElectronExtensionsClient::GetPermittedChromeSchemeHosts(
    const extensions::Extension* extension,
    const extensions::APIPermissionSet& api_permissions) const {
  return extensions::URLPatternSet();
}

bool ElectronExtensionsClient::IsScriptableURL(const GURL& url,
                                               std::string* error) const {
  // 对URL没有限制。
  return true;
}

const GURL& ElectronExtensionsClient::GetWebstoreBaseURL() const {
  return webstore_base_url_;
}

const GURL& ElectronExtensionsClient::GetWebstoreUpdateURL() const {
  return webstore_update_url_;
}

bool ElectronExtensionsClient::IsBlacklistUpdateURL(const GURL& url) const {
  return false;
}

}  // 命名空间电子
