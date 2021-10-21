// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_WEB_UI_CONTROLLER_FACTORY_H_
#define SHELL_BROWSER_ELECTRON_WEB_UI_CONTROLLER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "content/public/browser/web_ui_controller_factory.h"

namespace content {
class WebUIController;
}

namespace electron {

class ElectronWebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  static ElectronWebUIControllerFactory* GetInstance();

  ElectronWebUIControllerFactory();
  ~ElectronWebUIControllerFactory() override;

  // 内容：：WebUIControllerFactory：
  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) override;
  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) override;
  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) override;

 private:
  friend struct base::DefaultSingletonTraits<ElectronWebUIControllerFactory>;

  DISALLOW_COPY_AND_ASSIGN(ElectronWebUIControllerFactory);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_ELECTRON_WEB_UI_CONTROLLER_FACTORY_H_
