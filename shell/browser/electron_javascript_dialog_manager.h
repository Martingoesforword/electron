// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_JAVASCRIPT_DIALOG_MANAGER_H_
#define SHELL_BROWSER_ELECTRON_JAVASCRIPT_DIALOG_MANAGER_H_

#include <map>
#include <string>

#include "content/public/browser/javascript_dialog_manager.h"

namespace content {
class WebContents;
}

namespace electron {

class ElectronJavaScriptDialogManager
    : public content::JavaScriptDialogManager {
 public:
  ElectronJavaScriptDialogManager();
  ~ElectronJavaScriptDialogManager() override;

  // 内容：：JavaScriptDialogManager实现。
  void RunJavaScriptDialog(content::WebContents* web_contents,
                           content::RenderFrameHost* rfh,
                           content::JavaScriptDialogType dialog_type,
                           const std::u16string& message_text,
                           const std::u16string& default_prompt_text,
                           DialogClosedCallback callback,
                           bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(content::WebContents* web_contents,
                             content::RenderFrameHost* rfh,
                             bool is_reload,
                             DialogClosedCallback callback) override;
  void CancelDialogs(content::WebContents* web_contents,
                     bool reset_state) override;

 private:
  void OnMessageBoxCallback(DialogClosedCallback callback,
                            const std::string& origin,
                            int code,
                            bool checkbox_checked);

  std::map<std::string, int> origin_counts_;
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_ELECTRON_JAVASCRIPT_DIALOG_MANAGER_H_
