// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_SYSTEM_FACTORY_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_SYSTEM_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "extensions/browser/extension_system_provider.h"

namespace extensions {

// 提供ElectronExtensionSystem的工厂。
class ElectronExtensionSystemFactory : public ExtensionSystemProvider {
 public:
  // ExtensionSystemProvider实现：
  ExtensionSystem* GetForBrowserContext(
      content::BrowserContext* context) override;

  static ElectronExtensionSystemFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<ElectronExtensionSystemFactory>;

  ElectronExtensionSystemFactory();
  ~ElectronExtensionSystemFactory() override;

  // BrowserContextKeyedServiceFactory实现：
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;

  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionSystemFactory);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_SYSTEM_FACTORY_H_
