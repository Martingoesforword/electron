// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/electron_extensions_browser_api_provider.h"

#include "extensions/browser/extension_function_registry.h"
#include "shell/browser/extensions/api/generated_api_registration.h"
#include "shell/browser/extensions/api/i18n/i18n_api.h"
#include "shell/browser/extensions/api/tabs/tabs_api.h"

namespace extensions {

ElectronExtensionsBrowserAPIProvider::ElectronExtensionsBrowserAPIProvider() =
    default;
ElectronExtensionsBrowserAPIProvider::~ElectronExtensionsBrowserAPIProvider() =
    default;

void ElectronExtensionsBrowserAPIProvider::RegisterExtensionFunctions(
    ExtensionFunctionRegistry* registry) {
  // 从Electron生成的API。
  api::ElectronGeneratedFunctionRegistry::RegisterAll(registry);
}

}  // 命名空间扩展
