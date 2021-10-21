// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_FACTORY_H_
#define SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "shell/browser/serial/serial_chooser_context.h"

namespace electron {

class SerialChooserContext;

class SerialChooserContextFactory : public BrowserContextKeyedServiceFactory {
 public:
  static SerialChooserContext* GetForBrowserContext(
      content::BrowserContext* context);
  static SerialChooserContextFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<SerialChooserContextFactory>;

  SerialChooserContextFactory();
  ~SerialChooserContextFactory() override;

  // BrowserContextKeyedServiceFactory方法：
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(SerialChooserContextFactory);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_FACTORY_H_
