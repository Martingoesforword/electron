// 版权所有(C)2016年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_H_
#define SHELL_BROWSER_UI_VIEWS_WIN_CAPTION_BUTTON_H_

#include "chrome/browser/ui/view_ids.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/button/button.h"

namespace electron {

class WinFrameView;

class WinCaptionButton : public views::Button {
 public:
  WinCaptionButton(PressedCallback callback,
                   WinFrameView* frame_view,
                   ViewID button_type,
                   const std::u16string& accessible_name);
  WinCaptionButton(const WinCaptionButton&) = delete;
  WinCaptionButton& operator=(const WinCaptionButton&) = delete;

  // //视图：：按钮：
  gfx::Size CalculatePreferredSize() const override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void PaintButtonContents(gfx::Canvas* canvas) override;

  // 私人：
  // 返回我们应该在左侧直观保留的金额(RTL中的右侧)。
  // 用于按钮之间的间距。我们这样做，而不是重新定位。
  // 按钮，以避免可能导致的狭长死区。
  int GetBetweenButtonSpacing() const;

  // 返回此按钮的显示顺序(0为。
  // 向左绘制得最远，向右绘制较大的索引。
  // 较小的指数)。
  int GetButtonDisplayOrderIndex() const;

  // 用于按钮符号和背景混合的基色。用途。
  // 黑白的可读性越强。
  SkColor GetBaseColor() const;

  // 绘制按钮的最小化/最大化/恢复/关闭图标。
  void PaintSymbol(gfx::Canvas* canvas);

  WinFrameView* frame_view_;
  ViewID button_type_;
};
}  // 命名空间电子。

#endif  // Shell_Browser_UI_Views_Win_Caption_Button_H_
