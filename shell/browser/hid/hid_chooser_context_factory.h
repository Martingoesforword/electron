// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_FACTORY_H_
#define SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_FACTORY_H_

#include "base/macros.h"
#include "base/no_destructor.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace electron {

class HidChooserContext;

class HidChooserContextFactory : public BrowserContextKeyedServiceFactory {
 public:
  static HidChooserContext* GetForBrowserContext(
      content::BrowserContext* context);
  static HidChooserContext* GetForBrowserContextIfExists(
      content::BrowserContext* context);
  static HidChooserContextFactory* GetInstance();

 private:
  friend base::NoDestructor<HidChooserContextFactory>;

  HidChooserContextFactory();
  ~HidChooserContextFactory() override;

  // BrowserContextKeyedBaseFactory：
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  void BrowserContextShutdown(content::BrowserContext* context) override;

  DISALLOW_COPY_AND_ASSIGN(HidChooserContextFactory);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_FACTORY_H_
