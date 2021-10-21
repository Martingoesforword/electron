// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_AUTOFILL_POPUP_VIEW_H_
#define SHELL_BROWSER_UI_VIEWS_AUTOFILL_POPUP_VIEW_H_

#include <memory>

#include "shell/browser/ui/autofill_popup.h"

#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_widget_host.h"
#include "electron/buildflags/buildflags.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/views/drag_controller.h"
#include "ui/views/focus/widget_focus_manager.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

#if BUILDFLAG(ENABLE_OSR)
#include "shell/browser/osr/osr_view_proxy.h"
#endif

namespace electron {

const int kPopupBorderThickness = 1;
const int kSmallerFontSizeDelta = -1;
const int kEndPadding = 8;
const int kNamePadding = 15;
const int kRowHeight = 24;

class AutofillPopup;

// 仅用于触发辅助功能事件的子视图。处理渲染。
// By|AutofulPopupViews|。
class AutofillPopupChildView : public views::View {
 public:
  explicit AutofillPopupChildView(const std::u16string& suggestion)
      : suggestion_(suggestion) {
    SetFocusBehavior(FocusBehavior::ALWAYS);
  }

 private:
  ~AutofillPopupChildView() override {}

  // 视图：：视图实现。
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  std::u16string suggestion_;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopupChildView);
};

class AutofillPopupView : public views::WidgetDelegateView,
                          public views::WidgetFocusChangeListener,
                          public views::WidgetObserver,
                          public views::DragController {
 public:
  explicit AutofillPopupView(AutofillPopup* popup,
                             views::Widget* parent_widget);
  ~AutofillPopupView() override;

  void Show();
  void Hide();

  void OnSuggestionsChanged();

  int GetSelectedLine() { return selected_line_.value_or(-1); }

  void WriteDragDataForView(views::View*,
                            const gfx::Point&,
                            ui::OSExchangeData*) override;
  int GetDragOperationsForView(views::View*, const gfx::Point&) override;
  bool CanStartDragForView(views::View*,
                           const gfx::Point&,
                           const gfx::Point&) override;

 private:
  friend class AutofillPopup;

  void OnSelectedRowChanged(absl::optional<int> previous_row_selection,
                            absl::optional<int> current_row_selection);

  // 在|entry_rect|中绘制给定的自动填充条目。
  void DrawAutofillEntry(gfx::Canvas* canvas,
                         int index,
                         const gfx::Rect& entry_rect);

  // 根据|CONTROLLER_|给出的建议创建子视图。这些。
  // 子视图仅用于辅助功能事件。我们需要儿童视角来。
  // 当用户选择建议时，填写正确的|AXNodeData|。
  void CreateChildViews();

  void DoUpdateBoundsAndRedrawPopup();

  // 视图：：视图实现。
  void OnPaint(gfx::Canvas* canvas) override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  void OnMouseCaptureLost() override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void OnMouseMoved(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool HandleKeyPressEvent(const content::NativeWebKeyboardEvent& event);

  // 视图：：WidgetFocusChangeListener实现。
  void OnNativeFocusChanged(gfx::NativeView focused_now) override;

  // 视图：：WidgetWatch实现。
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

  void AcceptSuggestion(int index);
  bool AcceptSelectedLine();
  void AcceptSelection(const gfx::Point& point);
  void SetSelectedLine(absl::optional<int> selected_line);
  void SetSelection(const gfx::Point& point);
  void SelectNextLine();
  void SelectPreviousLine();
  void ClearSelection();

  // 停止观察小部件。
  void RemoveObserver();

  // 此弹出窗口的控制器。弱引用。
  AutofillPopup* popup_;

  // 触发此弹出窗口的窗口小部件。弱引用。
  views::Widget* parent_widget_;

  // 弹出窗口显示的时间。
  base::Time show_time_;

  // 当前选定行的索引。
  absl::optional<int> selected_line_;

#if BUILDFLAG(ENABLE_OSR)
  std::unique_ptr<OffscreenViewProxy> view_proxy_;
#endif

  // 注册的按键回调，负责接通线路。
  // 按键。
  content::RenderWidgetHost::KeyPressEventCallback keypress_callback_;

  base::WeakPtrFactory<AutofillPopupView> weak_ptr_factory_{this};
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Views_AutoFill_Popup_VIEW_H_
