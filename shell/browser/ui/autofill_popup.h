// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_AUTOFILL_POPUP_H_
#define SHELL_BROWSER_UI_AUTOFILL_POPUP_H_

#include <vector>

#include "content/public/browser/render_frame_host.h"
#include "shell/browser/ui/views/autofill_popup_view.h"
#include "ui/gfx/font_list.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace electron {

class AutofillPopupView;

class AutofillPopup : public views::ViewObserver {
 public:
  AutofillPopup();
  ~AutofillPopup() override;

  void CreateView(content::RenderFrameHost* render_frame,
                  content::RenderFrameHost* embedder_frame,
                  bool offscreen,
                  views::View* parent,
                  const gfx::RectF& bounds);
  void Hide();

  void SetItems(const std::vector<std::u16string>& values,
                const std::vector<std::u16string>& labels);
  void UpdatePopupBounds();

  gfx::Rect popup_bounds_in_view();

 private:
  friend class AutofillPopupView;

  // 视图：：视图观察者：
  void OnViewBoundsChanged(views::View* view) override;
  void OnViewIsDeleting(views::View* view) override;

  void AcceptSuggestion(int index);

  int GetDesiredPopupHeight();
  int GetDesiredPopupWidth();
  gfx::Rect GetRowBounds(int i);
  const gfx::FontList& GetValueFontListForRow(int index) const;
  const gfx::FontList& GetLabelFontListForRow(int index) const;
  ui::NativeTheme::ColorId GetBackgroundColorIDForRow(int index) const;

  int GetLineCount();
  std::u16string GetValueAt(int i);
  std::u16string GetLabelAt(int i);
  int LineFromY(int y) const;

  int selected_index_;

  // 弹出位置。
  gfx::Rect popup_bounds_;

  // 自动填充元素的边界。
  gfx::Rect element_bounds_;

  // 数据列表建议。
  std::vector<std::u16string> values_;
  std::vector<std::u16string> labels_;

  // 建议的字体列表。
  gfx::FontList smaller_font_list_;
  gfx::FontList bold_font_list_;

  // 用于将接受的建议发送到渲染帧，该渲染帧。
  // 请求打开弹出窗口。
  content::RenderFrameHost* frame_host_ = nullptr;

  // 弹出视图。生命周期由拥有的小部件管理。
  AutofillPopupView* view_ = nullptr;

  // 弹出式视图在其上显示的父视图。弱小的裁判。
  views::View* parent_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AutofillPopup);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_AutoFill_Popup_H_
