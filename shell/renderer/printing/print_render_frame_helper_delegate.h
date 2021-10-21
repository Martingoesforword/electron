// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_PRINTING_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_
#define SHELL_RENDERER_PRINTING_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_

#include "base/macros.h"
#include "components/printing/renderer/print_render_frame_helper.h"

namespace electron {

class PrintRenderFrameHelperDelegate
    : public printing::PrintRenderFrameHelper::Delegate {
 public:
  PrintRenderFrameHelperDelegate();
  ~PrintRenderFrameHelperDelegate() override;

 private:
  // 正在打印：：PrintRenderFrameHelper：：Delegate：
  blink::WebElement GetPdfElement(blink::WebLocalFrame* frame) override;
  bool IsPrintPreviewEnabled() override;
  bool OverridePrint(blink::WebLocalFrame* frame) override;

  DISALLOW_COPY_AND_ASSIGN(PrintRenderFrameHelperDelegate);
};

}  // 命名空间电子。

#endif  // SHELL_RENDERER_PRINTING_PRINT_RENDER_FRAME_HELPER_DELEGATE_H_
