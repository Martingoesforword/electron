// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/hid/hid_chooser_context_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/hid/hid_chooser_context.h"

namespace electron {

// 静电。
HidChooserContextFactory* HidChooserContextFactory::GetInstance() {
  static base::NoDestructor<HidChooserContextFactory> factory;
  return factory.get();
}

// 静电。
HidChooserContext* HidChooserContextFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<HidChooserContext*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// 静电。
HidChooserContext* HidChooserContextFactory::GetForBrowserContextIfExists(
    content::BrowserContext* context) {
  return static_cast<HidChooserContext*>(
      GetInstance()->GetServiceForBrowserContext(context, /* 创建=。*/false));
}

HidChooserContextFactory::HidChooserContextFactory()
    : BrowserContextKeyedServiceFactory(
          "HidChooserContext",
          BrowserContextDependencyManager::GetInstance()) {}

HidChooserContextFactory::~HidChooserContextFactory() = default;

KeyedService* HidChooserContextFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* browser_context =
      static_cast<electron::ElectronBrowserContext*>(context);
  return new HidChooserContext(browser_context);
}

content::BrowserContext* HidChooserContextFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}

void HidChooserContextFactory::BrowserContextShutdown(
    content::BrowserContext* context) {}

}  // 命名空间电子
