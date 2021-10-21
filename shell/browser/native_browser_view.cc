// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/native_browser_view.h"

#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents.h"

namespace electron {

NativeBrowserView::NativeBrowserView(
    InspectableWebContents* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents) {
  Observe(inspectable_web_contents_->GetWebContents());
}

NativeBrowserView::~NativeBrowserView() = default;

InspectableWebContentsView* NativeBrowserView::GetInspectableWebContentsView() {
  if (!inspectable_web_contents_)
    return nullptr;
  return inspectable_web_contents_->GetView();
}

void NativeBrowserView::WebContentsDestroyed() {
  inspectable_web_contents_ = nullptr;
}

}  // 命名空间电子
