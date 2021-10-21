// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/electron_browser_context_keyed_service_factories.h"

#include "extensions/browser/updater/update_service_factory.h"
#include "shell/browser/extensions/electron_extension_system_factory.h"

namespace extensions {
namespace electron {

void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  // TODO(Rockot)：一旦所有用户都支持UpdateService，就将其删除。
  // 扩展嵌入器(即Chrome。)。
  UpdateServiceFactory::GetInstance();

  ElectronExtensionSystemFactory::GetInstance();
}

}  // 命名空间电子。
}  // 命名空间扩展
