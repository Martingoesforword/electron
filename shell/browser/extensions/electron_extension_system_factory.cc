// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/electron_extension_system_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/extension_registry_factory.h"
#include "shell/browser/extensions/electron_extension_system.h"

using content::BrowserContext;

namespace extensions {

ExtensionSystem* ElectronExtensionSystemFactory::GetForBrowserContext(
    BrowserContext* context) {
  return static_cast<ElectronExtensionSystem*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// 静电。
ElectronExtensionSystemFactory* ElectronExtensionSystemFactory::GetInstance() {
  return base::Singleton<ElectronExtensionSystemFactory>::get();
}

ElectronExtensionSystemFactory::ElectronExtensionSystemFactory()
    : ExtensionSystemProvider("ElectronExtensionSystem",
                              BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ExtensionPrefsFactory::GetInstance());
  DependsOn(ExtensionRegistryFactory::GetInstance());
}

ElectronExtensionSystemFactory::~ElectronExtensionSystemFactory() = default;

KeyedService* ElectronExtensionSystemFactory::BuildServiceInstanceFor(
    BrowserContext* context) const {
  return new ElectronExtensionSystem(context);
}

BrowserContext* ElectronExtensionSystemFactory::GetBrowserContextToUse(
    BrowserContext* context) const {
  // 使用单独的实例隐姓埋名。
  return context;
}

bool ElectronExtensionSystemFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return true;
}

}  // 命名空间扩展
