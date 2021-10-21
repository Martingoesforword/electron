// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/views/native_frame_view.h"

#include "shell/browser/native_window.h"

namespace electron {

const char NativeFrameView::kViewClassName[] = "ElectronNativeFrameView";

NativeFrameView::NativeFrameView(NativeWindow* window, views::Widget* widget)
    : views::NativeFrameView(widget), window_(window) {}

gfx::Size NativeFrameView::GetMinimumSize() const {
  return window_->GetMinimumSize();
}

gfx::Size NativeFrameView::GetMaximumSize() const {
  gfx::Size size = window_->GetMaximumSize();
  // 电子公共API在未设置最大大小时返回(0，0)，但它。
  // 会破坏内部窗口API，如HWNDMessageHandler：：SetAspectRatio。
  return size.IsEmpty() ? gfx::Size(INT_MAX, INT_MAX) : size;
}

const char* NativeFrameView::GetClassName() const {
  return kViewClassName;
}

}  // 命名空间电子
