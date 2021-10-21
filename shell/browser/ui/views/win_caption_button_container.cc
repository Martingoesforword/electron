// 版权所有2020年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/views/win_caption_button_container.h"

#include <memory>
#include <utility>

#include "shell/browser/ui/views/win_caption_button.h"
#include "shell/browser/ui/views/win_frame_view.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view_class_properties.h"

namespace electron {

namespace {

std::unique_ptr<WinCaptionButton> CreateCaptionButton(
    views::Button::PressedCallback callback,
    WinFrameView* frame_view,
    ViewID button_type,
    int accessible_name_resource_id) {
  return std::make_unique<WinCaptionButton>(
      std::move(callback), frame_view, button_type,
      l10n_util::GetStringUTF16(accessible_name_resource_id));
}

bool HitTestCaptionButton(WinCaptionButton* button, const gfx::Point& point) {
  return button && button->GetVisible() && button->bounds().Contains(point);
}

}  // 匿名命名空间。

WinCaptionButtonContainer::WinCaptionButtonContainer(WinFrameView* frame_view)
    : frame_view_(frame_view),
      minimize_button_(AddChildView(CreateCaptionButton(
          base::BindRepeating(&views::Widget::Minimize,
                              base::Unretained(frame_view_->frame())),
          frame_view_,
          VIEW_ID_MINIMIZE_BUTTON,
          IDS_APP_ACCNAME_MINIMIZE))),
      maximize_button_(AddChildView(CreateCaptionButton(
          base::BindRepeating(&views::Widget::Maximize,
                              base::Unretained(frame_view_->frame())),
          frame_view_,
          VIEW_ID_MAXIMIZE_BUTTON,
          IDS_APP_ACCNAME_MAXIMIZE))),
      restore_button_(AddChildView(CreateCaptionButton(
          base::BindRepeating(&views::Widget::Restore,
                              base::Unretained(frame_view_->frame())),
          frame_view_,
          VIEW_ID_RESTORE_BUTTON,
          IDS_APP_ACCNAME_RESTORE))),
      close_button_(AddChildView(CreateCaptionButton(
          base::BindRepeating(&views::Widget::CloseWithReason,
                              base::Unretained(frame_view_->frame()),
                              views::Widget::ClosedReason::kCloseButtonClicked),
          frame_view_,
          VIEW_ID_CLOSE_BUTTON,
          IDS_APP_ACCNAME_CLOSE))) {
  // 布局是水平的，按钮放置在视图的后端。
  // 这允许容器扩展为人造标题栏/拖动手柄。
  auto* const layout = SetLayoutManager(std::make_unique<views::FlexLayout>());
  layout->SetOrientation(views::LayoutOrientation::kHorizontal)
      .SetMainAxisAlignment(views::LayoutAlignment::kEnd)
      .SetCrossAxisAlignment(views::LayoutAlignment::kStart)
      .SetDefault(
          views::kFlexBehaviorKey,
          views::FlexSpecification(views::LayoutOrientation::kHorizontal,
                                   views::MinimumFlexSizeRule::kPreferred,
                                   views::MaximumFlexSizeRule::kPreferred,
                                   /* 为高度调整宽度。*/ false,
                                   views::MinimumFlexSizeRule::kScaleToZero));
}

WinCaptionButtonContainer::~WinCaptionButtonContainer() {}

int WinCaptionButtonContainer::NonClientHitTest(const gfx::Point& point) const {
  DCHECK(HitTestPoint(point))
      << "should only be called with a point inside this view's bounds";
  if (HitTestCaptionButton(minimize_button_, point)) {
    return HTMINBUTTON;
  }
  if (HitTestCaptionButton(maximize_button_, point)) {
    return HTMAXBUTTON;
  }
  if (HitTestCaptionButton(restore_button_, point)) {
    return HTMAXBUTTON;
  }
  if (HitTestCaptionButton(close_button_, point)) {
    return HTCLOSE;
  }
  return HTCAPTION;
}

void WinCaptionButtonContainer::ResetWindowControls() {
  minimize_button_->SetState(views::Button::STATE_NORMAL);
  maximize_button_->SetState(views::Button::STATE_NORMAL);
  restore_button_->SetState(views::Button::STATE_NORMAL);
  close_button_->SetState(views::Button::STATE_NORMAL);
  InvalidateLayout();
}

void WinCaptionButtonContainer::AddedToWidget() {
  views::Widget* const widget = GetWidget();

  DCHECK(!widget_observation_.IsObserving());
  widget_observation_.Observe(widget);

  UpdateButtons();

  if (frame_view_->window()->IsWindowControlsOverlayEnabled()) {
    SetPaintToLayer();
  }
}

void WinCaptionButtonContainer::RemovedFromWidget() {
  DCHECK(widget_observation_.IsObserving());
  widget_observation_.Reset();
}

void WinCaptionButtonContainer::OnWidgetBoundsChanged(
    views::Widget* widget,
    const gfx::Rect& new_bounds) {
  UpdateButtons();
}

void WinCaptionButtonContainer::UpdateButtons() {
  const bool is_maximized = frame_view_->frame()->IsMaximized();
  restore_button_->SetVisible(is_maximized);
  maximize_button_->SetVisible(!is_maximized);

  // 在触摸模式下，窗口无法退出全屏或平铺模式，因此。
  // 应禁用最大化/恢复按钮。
  const bool is_touch = ui::TouchUiController::Get()->touch_ui();
  restore_button_->SetEnabled(!is_touch);
  maximize_button_->SetEnabled(!is_touch);
  InvalidateLayout();
}
}  // 命名空间电子
