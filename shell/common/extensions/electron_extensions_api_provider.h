// 版权所有2018年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_API_PROVIDER_H_
#define SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_API_PROVIDER_H_

#include <string>

#include "base/macros.h"
#include "extensions/common/extensions_api_provider.h"

namespace electron {

class ElectronExtensionsAPIProvider : public extensions::ExtensionsAPIProvider {
 public:
  ElectronExtensionsAPIProvider();
  ~ElectronExtensionsAPIProvider() override;

  // 扩展APIProvider：
  void AddAPIFeatures(extensions::FeatureProvider* provider) override;
  void AddManifestFeatures(extensions::FeatureProvider* provider) override;
  void AddPermissionFeatures(extensions::FeatureProvider* provider) override;
  void AddBehaviorFeatures(extensions::FeatureProvider* provider) override;
  void AddAPIJSONSources(
      extensions::JSONFeatureProviderSource* json_source) override;
  bool IsAPISchemaGenerated(const std::string& name) override;
  base::StringPiece GetAPISchema(const std::string& name) override;
  void RegisterPermissions(
      extensions::PermissionsInfo* permissions_info) override;
  void RegisterManifestHandlers() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionsAPIProvider);
};

}  // 命名空间电子。

#endif  // SHELL_COMMON_EXTENSIONS_ELECTRON_EXTENSIONS_API_PROVIDER_H_
