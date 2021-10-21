// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/serial/serial_chooser_context_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/serial/serial_chooser_context.h"

namespace electron {

SerialChooserContextFactory::SerialChooserContextFactory()
    : BrowserContextKeyedServiceFactory(
          "SerialChooserContext",
          BrowserContextDependencyManager::GetInstance()) {}

SerialChooserContextFactory::~SerialChooserContextFactory() = default;

KeyedService* SerialChooserContextFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new SerialChooserContext();
}

// 静电。
SerialChooserContextFactory* SerialChooserContextFactory::GetInstance() {
  return base::Singleton<SerialChooserContextFactory>::get();
}

// 静电。
SerialChooserContext* SerialChooserContextFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SerialChooserContext*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

content::BrowserContext* SerialChooserContextFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}

}  // 命名空间电子
