// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_

namespace extensions {
namespace electron {

// 确保存在由提供的任何BrowserContextKeyedServiceFactory。
// 核心扩展代码。
void EnsureBrowserContextKeyedServiceFactoriesBuilt();

}  // 命名空间电子。
}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_BROWSER_CONTEXT_KEYED_SERVICE_FACTORIES_H_
