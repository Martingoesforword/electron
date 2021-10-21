// 版权所有2020 Microsoft，Inc.保留所有权利。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/printing/print_view_manager_electron.h"

#include <utility>

#include "build/build_config.h"

namespace electron {

PrintViewManagerElectron::PrintViewManagerElectron(
    content::WebContents* web_contents)
    : PrintViewManagerBase(web_contents) {}

PrintViewManagerElectron::~PrintViewManagerElectron() = default;

// 静电。
void PrintViewManagerElectron::BindPrintManagerHost(
    mojo::PendingAssociatedReceiver<printing::mojom::PrintManagerHost> receiver,
    content::RenderFrameHost* rfh) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents)
    return;
  auto* print_manager = PrintViewManagerElectron::FromWebContents(web_contents);
  if (!print_manager)
    return;
  print_manager->BindReceiver(std::move(receiver), rfh);
}

void PrintViewManagerElectron::SetupScriptedPrintPreview(
    SetupScriptedPrintPreviewCallback callback) {
  std::move(callback).Run();
}

void PrintViewManagerElectron::ShowScriptedPrintPreview(
    bool source_is_modifiable) {}

void PrintViewManagerElectron::RequestPrintPreview(
    printing::mojom::RequestPrintPreviewParamsPtr params) {}

void PrintViewManagerElectron::CheckForCancel(int32_t preview_ui_id,
                                              int32_t request_id,
                                              CheckForCancelCallback callback) {
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PrintViewManagerElectron)

}  // 命名空间电子
