// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。
// 
// 此文件的某些部分源自。
// Chrome/browser/ui/views/frame/glass_browser_frame_view.cc，
// 版权所有(C)2012 Chromium作者，
// 它由BSD样式的许可证管理。

#include "shell/browser/ui/views/win_frame_view.h"

#include <dwmapi.h>
#include <memory>

#include "base/win/windows_version.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/views/win_caption_button_container.h"
#include "ui/base/win/hwnd_metrics.h"
#include "ui/display/win/dpi.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/views/widget/widget.h"
#include "ui/views/win/hwnd_util.h"

namespace electron {

const char WinFrameView::kViewClassName[] = "WinFrameView";

WinFrameView::WinFrameView() = default;

WinFrameView::~WinFrameView() = default;

void WinFrameView::Init(NativeWindowViews* window, views::Widget* frame) {
  window_ = window;
  frame_ = frame;

  if (window->IsWindowControlsOverlayEnabled()) {
    caption_button_container_ =
        AddChildView(std::make_unique<WinCaptionButtonContainer>(this));
  } else {
    caption_button_container_ = nullptr;
  }
}

SkColor WinFrameView::GetReadableFeatureColor(SkColor background_color) {
  // 这里不使用color_utils：：GetColorWithMaxContrast()/IsDark()是因为。
  // 它们基于Chrome亮/暗端点进行切换，而我们希望使用。
  // 下面的系统本机行为。
  const auto windows_luma = [](SkColor c) {
    return 0.25f * SkColorGetR(c) + 0.625f * SkColorGetG(c) +
           0.125f * SkColorGetB(c);
  };
  return windows_luma(background_color) <= 128.0f ? SK_ColorWHITE
                                                  : SK_ColorBLACK;
}

gfx::Rect WinFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  return views::GetWindowBoundsForClientBounds(
      static_cast<views::View*>(const_cast<WinFrameView*>(this)),
      client_bounds);
}

int WinFrameView::FrameBorderThickness() const {
  return (IsMaximized() || frame()->IsFullscreen())
             ? 0
             : display::win::ScreenWin::GetSystemMetricsInDIP(SM_CXSIZEFRAME);
}

int WinFrameView::NonClientHitTest(const gfx::Point& point) {
  if (window_->has_frame())
    return frame_->client_view()->NonClientHitTest(point);

  if (ShouldCustomDrawSystemTitlebar()) {
    // 查看该点是否在任何窗口控件内。
    if (caption_button_container_) {
      gfx::Point local_point = point;

      ConvertPointToTarget(parent(), caption_button_container_, &local_point);
      if (caption_button_container_->HitTestPoint(local_point)) {
        const int hit_test_result =
            caption_button_container_->NonClientHitTest(local_point);
        if (hit_test_result != HTNOWHERE)
          return hit_test_result;
      }
    }

    // 在Windows 8+上，字幕按钮几乎与右上角对接。
    // 窗户的一角。此代码确保鼠标未设置为。
    // 光标悬停在标题按钮上时，会给出不正确的。
    // 用户可以调整窗口大小的印象。
    if (base::win::GetVersion() >= base::win::Version::WIN8) {
      RECT button_bounds = {0};
      if (SUCCEEDED(DwmGetWindowAttribute(
              views::HWNDForWidget(frame()), DWMWA_CAPTION_BUTTON_BOUNDS,
              &button_bounds, sizeof(button_bounds)))) {
        gfx::RectF button_bounds_in_dips = gfx::ConvertRectToDips(
            gfx::Rect(button_bounds), display::win::GetDPIScale());
        // TODO(crbug.com/1131681)：GetMirroredRect()需要整数RECT，
        // 但DIPS中的大小不能是带有小数器件的整数。
        // 比例因子。如果我们想继续使用整数，可以选择使用。
        // ToFlooredRectDeproated()似乎做错了事情，因为。
        // 下面关于嵌入1个DIP而不是1个物理像素的评论。我们。
        // 应该使用ToEnclosedRect()，然后我们可以将inset 1。
        // 这里是物理像素。
        gfx::Rect buttons = GetMirroredRect(
            gfx::ToFlooredRectDeprecated(button_bounds_in_dips));

        // 中的标题按钮正上方有一个小的单像素条带。
        // 调整大小的边框通过它“窥视”。
        constexpr int kCaptionButtonTopInset = 1;
        // 标题按钮上方窗口边缘的大小调整区域为。
        // 1像素，不考虑比例因子。如果我们在转换前插入1。
        // 对于下沉，精度损失可能会完全消除这一区域。这个。
        // 我们能做的最好的事情就是在转换后插入。这保证了我们会。
        // 在可以调整大小时显示调整大小光标。它的成本。
        // 也可能会显示在下沉部分，而不是。
        // 最外面的像素。
        buttons.Inset(0, kCaptionButtonTopInset, 0, 0);
        if (buttons.Contains(point))
          return HTNOWHERE;
      }
    }

    int top_border_thickness = FrameTopBorderThickness(false);
    // 在窗口角落，调整大小的区域实际上并不大，而是16。
    // 顶端和底端的像素会触发对角大小调整。
    constexpr int kResizeCornerWidth = 16;
    int window_component = GetHTComponentForFrame(
        point, gfx::Insets(top_border_thickness, 0, 0, 0), top_border_thickness,
        kResizeCornerWidth - FrameBorderThickness(),
        frame()->widget_delegate()->CanResize());
    if (window_component != HTNOWHERE)
      return window_component;
  }

  // 最后使用父类的命中测试。
  return FramelessView::NonClientHitTest(point);
}

const char* WinFrameView::GetClassName() const {
  return kViewClassName;
}

bool WinFrameView::IsMaximized() const {
  return frame()->IsMaximized();
}

bool WinFrameView::ShouldCustomDrawSystemTitlebar() const {
  return window()->IsWindowControlsOverlayEnabled();
}

void WinFrameView::Layout() {
  LayoutCaptionButtons();
  if (window()->IsWindowControlsOverlayEnabled()) {
    LayoutWindowControlsOverlay();
  }
  NonClientFrameView::Layout();
}

int WinFrameView::FrameTopBorderThickness(bool restored) const {
  // 鼠标和触摸位置是四舍五入的，但GetSystemMetricsInDIP是四舍五入的，
  // 所以我们需要停在地板上，否则这一差异将导致最受欢迎的。
  // 应该成功的时候却失败了。
  return std::floor(
      FrameTopBorderThicknessPx(restored) /
      display::win::ScreenWin::GetScaleFactorForHWND(HWNDForView(this)));
}

int WinFrameView::FrameTopBorderThicknessPx(bool restored) const {
  // 与FrameBorderThickness()不同，因为我们不能插入顶部。
  // 边框，否则Windows将给我们一个标准标题栏。
  // 对于最大化窗口，情况并非如此，上边框必须为。
  // 插入以避免与上面的显示器重叠。

  // 请参阅BrowserDesktopWindowTreeHostWin：：GetClientAreaInsets().中的注释。
  const bool needs_no_border =
      (ShouldCustomDrawSystemTitlebar() && frame()->IsMaximized()) ||
      frame()->IsFullscreen();
  if (needs_no_border && !restored)
    return 0;

  // 请注意，此方法假定所有。
  // 窗户的侧面。
  // TODO(Dfred)：考虑让它返回一个gfx：：insets对象。
  return ui::GetFrameThickness(
      MonitorFromWindow(HWNDForView(this), MONITOR_DEFAULTTONEAREST));
}

int WinFrameView::TitlebarMaximizedVisualHeight() const {
  int maximized_height =
      display::win::ScreenWin::GetSystemMetricsInDIP(SM_CYCAPTION);
  return maximized_height;
}

int WinFrameView::TitlebarHeight(bool restored) const {
  if (frame()->IsFullscreen() && !restored)
    return 0;

  return TitlebarMaximizedVisualHeight() + FrameTopBorderThickness(false);
}

int WinFrameView::WindowTopY() const {
  // 最大化时，窗口顶部为SM_CYSIZEFRAME像素(请参阅中的注释。
  // 恢复时的FrameTopBorderThickness()和Floor(系统dsf)像素。
  // 不幸的是，我们不能在HiDPI上代表这两个人中的任何一个。
  // 非整数凹陷，因此我们返回最接近的合理值。
  if (IsMaximized())
    return FrameTopBorderThickness(false);

  return 1;
}

void WinFrameView::LayoutCaptionButtons() {
  if (!caption_button_container_)
    return;

  // 非自定义系统标题栏已包含标题按钮。
  if (!ShouldCustomDrawSystemTitlebar()) {
    caption_button_container_->SetVisible(false);
    return;
  }

  caption_button_container_->SetVisible(true);

  const gfx::Size preferred_size =
      caption_button_container_->GetPreferredSize();
  int height = preferred_size.height();

  height = IsMaximized() ? TitlebarMaximizedVisualHeight()
                         : TitlebarHeight(false) - WindowTopY();

  // TODO(Mlaurencin)：这会在右侧之间创建1个像素的间距。
  // 覆盖的边缘和窗口的边缘，允许此边缘。
  // 部分返回正确的命中测试，并手动正确调整大小。
  // 可以探索替代方案，但视图结构的差异。
  // 在电子和铬之间的选择可能会导致这是最好的选择。
  int variable_width =
      IsMaximized() ? preferred_size.width() : preferred_size.width() - 1;
  caption_button_container_->SetBounds(width() - preferred_size.width(),
                                       WindowTopY(), variable_width, height);
}

void WinFrameView::LayoutWindowControlsOverlay() {
  int overlay_height = caption_button_container_->size().height();
  int overlay_width = caption_button_container_->size().width();
  int bounding_rect_width = width() - overlay_width;
  auto bounding_rect =
      GetMirroredRect(gfx::Rect(0, 0, bounding_rect_width, overlay_height));

  window()->SetWindowControlsOverlayRect(bounding_rect);
  window()->NotifyLayoutWindowControlsOverlay();
}

}  // 命名空间电子
