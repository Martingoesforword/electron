// 版权所有(C)2020 Microsoft，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_WEBUI_ACCESSIBILITY_UI_H_
#define SHELL_BROWSER_UI_WEBUI_ACCESSIBILITY_UI_H_

#include "base/macros.h"
#include "chrome/browser/accessibility/accessibility_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

// 控制辅助功能Web用户界面页面。
class ElectronAccessibilityUI : public content::WebUIController {
 public:
  explicit ElectronAccessibilityUI(content::WebUI* web_ui);
  ~ElectronAccessibilityUI() override;
};

// 管理通过json从accessibility.js发送的消息。
class ElectronAccessibilityUIMessageHandler
    : public AccessibilityUIMessageHandler {
 public:
  ElectronAccessibilityUIMessageHandler();

  void RegisterMessages() final;

 private:
  void RequestNativeUITree(const base::ListValue* args);

  DISALLOW_COPY_AND_ASSIGN(ElectronAccessibilityUIMessageHandler);
};

#endif  // Shell_Browser_UI_WebUI_Accessibility_UI_H_
