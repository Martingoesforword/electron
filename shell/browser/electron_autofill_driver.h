// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_AUTOFILL_DRIVER_H_
#define SHELL_BROWSER_ELECTRON_AUTOFILL_DRIVER_H_

#include <memory>
#include <vector>

#if defined(TOOLKIT_VIEWS)
#include "shell/browser/ui/autofill_popup.h"
#endif

#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "shell/common/api/api.mojom.h"

namespace electron {

class AutofillDriver : public mojom::ElectronAutofillDriver {
 public:
  AutofillDriver(
      content::RenderFrameHost* render_frame_host,
      mojo::PendingAssociatedReceiver<mojom::ElectronAutofillDriver> request);

  ~AutofillDriver() override;

  void ShowAutofillPopup(const gfx::RectF& bounds,
                         const std::vector<std::u16string>& values,
                         const std::vector<std::u16string>& labels) override;
  void HideAutofillPopup() override;

 private:
  content::RenderFrameHost* const render_frame_host_;

#if defined(TOOLKIT_VIEWS)
  std::unique_ptr<AutofillPopup> autofill_popup_;
#endif

  mojo::AssociatedReceiver<mojom::ElectronAutofillDriver> receiver_;
};

}  // 命名空间电子。

#endif  // 外壳浏览器电子自动填充驱动程序H_
