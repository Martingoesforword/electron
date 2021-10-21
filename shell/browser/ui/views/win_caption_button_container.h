// 版权所有2020年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_CONTAINER_H_
#define SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_CONTAINER_H_

#include "base/scoped_observation.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/pointer/touch_ui_controller.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace electron {

class WinFrameView;
class WinCaptionButton;

// 为Windows 10标题按钮提供容器，这些按钮可以在。
// 根据需要设置框架和浏览器窗口。当水平扩展时，将成为。
// 用于移动窗户的抓取杆。
class WinCaptionButtonContainer : public views::View,
                                  public views::WidgetObserver {
 public:
  explicit WinCaptionButtonContainer(WinFrameView* frame_view);
  ~WinCaptionButtonContainer() override;

  // 测试以查看指定的|point|(在此视图的。
  // 坐标且必须在此视图的边界内)在以下其中之一。
  // 标题按钮。返回中定义的HitTestCompat枚举之一。
  // Ui/base/hit_test.h，如果区域点击将是窗口的一部分，则返回HTCAPTION。
  // 拖动控制柄，否则拖动HTNOWHERE。
  // 另请参阅ClientView：：NonClientHitTest。
  int NonClientHitTest(const gfx::Point& point) const;

 private:
  // 视图：：视图：
  void AddedToWidget() override;
  void RemovedFromWidget() override;

  // 视图：：WidgetViewer：
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

  void ResetWindowControls();

  // 根据窗口状态设置标题按钮的可见性和启用状态。
  // 同时只能看到最大化或恢复按钮中的一个。
  // 时间，两者在Tablet UI模式下都被禁用。
  void UpdateButtons();

  WinFrameView* const frame_view_;
  WinCaptionButton* const minimize_button_;
  WinCaptionButton* const maximize_button_;
  WinCaptionButton* const restore_button_;
  WinCaptionButton* const close_button_;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};

  base::CallbackListSubscription subscription_ =
      ui::TouchUiController::Get()->RegisterCallback(
          base::BindRepeating(&WinCaptionButtonContainer::UpdateButtons,
                              base::Unretained(this)));
};
}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_CONTAINER_H_
