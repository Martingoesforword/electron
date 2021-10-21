// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/views/frameless_view.h"

#include "shell/browser/native_browser_view_views.h"
#include "shell/browser/native_window_views.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace electron {

namespace {

const int kResizeInsideBoundsSize = 5;
const int kResizeAreaCornerSize = 16;

}  // 命名空间。

// 静电。
const char FramelessView::kViewClassName[] = "FramelessView";

FramelessView::FramelessView() = default;

FramelessView::~FramelessView() = default;

void FramelessView::Init(NativeWindowViews* window, views::Widget* frame) {
  window_ = window;
  frame_ = frame;
}

int FramelessView::ResizingBorderHitTest(const gfx::Point& point) {
  // 请先检查框架，因为我们允许有一小块区域与内容重叠。
  // 用于调整手柄的大小。
  bool can_ever_resize = frame_->widget_delegate()
                             ? frame_->widget_delegate()->CanResize()
                             : false;

  // Https://github.com/electron/electron/issues/611。
  // 如果窗口不可调整大小，则应始终返回HTNOWHERE，否则。
  // DOM的悬停状态可能不会被清除。
  if (!can_ever_resize)
    return HTNOWHERE;

  // 当窗口最大化时，不允许重叠调整大小手柄。
  // FullScreen，因为它不能在这些状态下调整大小。
  int resize_border = frame_->IsMaximized() || frame_->IsFullscreen()
                          ? 0
                          : kResizeInsideBoundsSize;
  return GetHTComponentForFrame(point, gfx::Insets(resize_border),
                                kResizeAreaCornerSize, kResizeAreaCornerSize,
                                can_ever_resize);
}

gfx::Rect FramelessView::GetBoundsForClientView() const {
  return bounds();
}

gfx::Rect FramelessView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  gfx::Rect window_bounds = client_bounds;
  // 强制最小大小(1，1)，以防与CLIENT_Bound一起传递。
  // 空大小。当无边框窗口正在运行时，可能会发生这种情况。
  // 已初始化。
  if (window_bounds.IsEmpty()) {
    window_bounds.set_width(1);
    window_bounds.set_height(1);
  }
  return window_bounds;
}

int FramelessView::NonClientHitTest(const gfx::Point& cursor) {
  if (frame_->IsFullscreen())
    return HTCLIENT;

  // 检查附加的BrowserViews中是否有潜在的可拖动区域。
  for (auto* view : window_->browser_views()) {
    auto* native_view = static_cast<NativeBrowserViewViews*>(view);
    auto* view_draggable_region = native_view->draggable_region();
    if (view_draggable_region &&
        view_draggable_region->contains(cursor.x(), cursor.y()))
      return HTCAPTION;
  }

  // 支持通过拖动边框来调整无框架窗口的大小。
  int frame_component = ResizingBorderHitTest(cursor);
  if (frame_component != HTNOWHERE)
    return frame_component;

  // 检查工作区中可能存在的可拖动区域，以查找无框架。
  // 窗户。
  SkRegion* draggable_region = window_->draggable_region();
  if (draggable_region && draggable_region->contains(cursor.x(), cursor.y()))
    return HTCAPTION;

  return HTCLIENT;
}

void FramelessView::GetWindowMask(const gfx::Size& size, SkPath* window_mask) {}

void FramelessView::ResetWindowControls() {}

void FramelessView::UpdateWindowIcon() {}

void FramelessView::UpdateWindowTitle() {}

void FramelessView::SizeConstraintsChanged() {}

gfx::Size FramelessView::CalculatePreferredSize() const {
  return frame_->non_client_view()
      ->GetWindowBoundsForClientBounds(
          gfx::Rect(frame_->client_view()->GetPreferredSize()))
      .size();
}

gfx::Size FramelessView::GetMinimumSize() const {
  return window_->GetContentMinimumSize();
}

gfx::Size FramelessView::GetMaximumSize() const {
  gfx::Size size = window_->GetContentMaximumSize();
  // 电子公共API在未设置最大大小时返回(0，0)，但它。
  // 会破坏内部窗口API，如HWNDMessageHandler：：SetAspectRatio。
  return size.IsEmpty() ? gfx::Size(INT_MAX, INT_MAX) : size;
}

const char* FramelessView::GetClassName() const {
  return kViewClassName;
}

}  // 命名空间电子
