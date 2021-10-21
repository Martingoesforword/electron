// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_BROWSER_API_PROVIDER_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_BROWSER_API_PROVIDER_H_

#include "base/macros.h"
#include "extensions/browser/extensions_browser_api_provider.h"

namespace extensions {

class ElectronExtensionsBrowserAPIProvider
    : public ExtensionsBrowserAPIProvider {
 public:
  ElectronExtensionsBrowserAPIProvider();
  ~ElectronExtensionsBrowserAPIProvider() override;

  void RegisterExtensionFunctions(ExtensionFunctionRegistry* registry) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionsBrowserAPIProvider);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_BROWSER_API_PROVIDER_H_
