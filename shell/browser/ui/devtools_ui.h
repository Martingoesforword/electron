// 版权所有(C)2011年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_UI_DEVTOOLS_UI_H_
#define SHELL_BROWSER_UI_DEVTOOLS_UI_H_

#include "base/compiler_specific.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_ui_controller.h"

namespace electron {

class DevToolsUI : public content::WebUIController {
 public:
  explicit DevToolsUI(content::BrowserContext* browser_context,
                      content::WebUI* web_ui);

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsUI);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_DevTools_UI_H_
