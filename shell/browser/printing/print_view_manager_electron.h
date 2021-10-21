// 版权所有2020 Microsoft，Inc.保留所有权利。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_
#define SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/printing/print_view_manager_base.h"
#include "content/public/browser/web_contents_user_data.h"

namespace electron {

class PrintViewManagerElectron
    : public printing::PrintViewManagerBase,
      public content::WebContentsUserData<PrintViewManagerElectron> {
 public:
  ~PrintViewManagerElectron() override;

  static void BindPrintManagerHost(
      mojo::PendingAssociatedReceiver<printing::mojom::PrintManagerHost>
          receiver,
      content::RenderFrameHost* rfh);

  void SetupScriptedPrintPreview(
      SetupScriptedPrintPreviewCallback callback) override;
  void ShowScriptedPrintPreview(bool source_is_modifiable) override;
  void RequestPrintPreview(
      printing::mojom::RequestPrintPreviewParamsPtr params) override;
  void CheckForCancel(int32_t preview_ui_id,
                      int32_t request_id,
                      CheckForCancelCallback callback) override;

 private:
  friend class content::WebContentsUserData<PrintViewManagerElectron>;
  explicit PrintViewManagerElectron(content::WebContents* web_contents);

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(PrintViewManagerElectron);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_PRINTING_PRINT_VIEW_MANAGER_ELECTRON_H_
