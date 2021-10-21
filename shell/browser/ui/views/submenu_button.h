// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_SUBMENU_BUTTON_H_
#define SHELL_BROWSER_UI_VIEWS_SUBMENU_BUTTON_H_

#include <memory>

#include "ui/accessibility/ax_node_data.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/controls/button/menu_button.h"

namespace electron {

// 菜单栏用来显示子菜单的特殊按钮。
class SubmenuButton : public views::MenuButton {
 public:
  SubmenuButton(PressedCallback callback,
                const std::u16string& title,
                const SkColor& background_color);
  ~SubmenuButton() override;

  void SetAcceleratorVisibility(bool visible);
  void SetUnderlineColor(SkColor color);

  char16_t accelerator() const { return accelerator_; }

  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // 视图：：菜单按钮：
  void PaintButtonContents(gfx::Canvas* canvas) override;

 private:
  bool GetUnderlinePosition(const std::u16string& text,
                            char16_t* accelerator,
                            int* start,
                            int* end) const;
  void GetCharacterPosition(const std::u16string& text,
                            int index,
                            int* pos) const;

  char16_t accelerator_ = 0;

  bool show_underline_ = false;

  int underline_start_ = 0;
  int underline_end_ = 0;
  int text_width_ = 0;
  int text_height_ = 0;
  SkColor underline_color_ = SK_ColorBLACK;
  SkColor background_color_;

  DISALLOW_COPY_AND_ASSIGN(SubmenuButton);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Views_SubMenu_Button_H_
