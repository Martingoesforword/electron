// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_CLIENT_H_
#define SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_CLIENT_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "extensions/common/extensions_client.h"
#include "url/gurl.h"

namespace extensions {
class APIPermissionSet;
class Extension;
class PermissionMessageProvider;
class PermissionIDSet;
class URLPatternSet;
}  // 命名空间扩展。

namespace electron {

// ExtensionsClient的app_shell实现。
class ElectronExtensionsClient : public extensions::ExtensionsClient {
 public:
  using ScriptingAllowlist = extensions::ExtensionsClient::ScriptingAllowlist;

  ElectronExtensionsClient();
  ~ElectronExtensionsClient() override;

  // 扩展客户端覆盖：
  void Initialize() override;
  void InitializeWebStoreUrls(base::CommandLine* command_line) override;
  const extensions::PermissionMessageProvider& GetPermissionMessageProvider()
      const override;
  const std::string GetProductName() override;
  void FilterHostPermissions(
      const extensions::URLPatternSet& hosts,
      extensions::URLPatternSet* new_hosts,
      extensions::PermissionIDSet* permissions) const override;
  void SetScriptingAllowlist(const ScriptingAllowlist& allowlist) override;
  const ScriptingAllowlist& GetScriptingAllowlist() const override;
  extensions::URLPatternSet GetPermittedChromeSchemeHosts(
      const extensions::Extension* extension,
      const extensions::APIPermissionSet& api_permissions) const override;
  bool IsScriptableURL(const GURL& url, std::string* error) const override;
  const GURL& GetWebstoreBaseURL() const override;
  const GURL& GetWebstoreUpdateURL() const override;
  bool IsBlacklistUpdateURL(const GURL& url) const override;

 private:
  ScriptingAllowlist scripting_allowlist_;

  const GURL webstore_base_url_;
  const GURL webstore_update_url_;

  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionsClient);
};

}  // 命名空间电子。

#endif  // SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_CLIENT_H_
