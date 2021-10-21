// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_
#define SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_

#include "shell/browser/native_window_observer.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/browser/ui/views/menu_delegate.h"
#include "shell/browser/ui/views/root_view.h"
#include "ui/views/accessible_pane_view.h"

namespace views {
class MenuButton;
}

namespace electron {

class MenuBar : public views::AccessiblePaneView,
                public MenuDelegate::Observer,
                public NativeWindowObserver {
 public:
  static const char kViewClassName[];

  MenuBar(NativeWindow* window, RootView* root_view);
  ~MenuBar() override;

  // 用新菜单替换当前菜单。
  void SetMenu(ElectronMenuModel* menu_model);

  // 在加速器下显示下划线。
  void SetAcceleratorVisibility(bool visible);

  // 如果子菜单有快捷键|键，则返回TRUE。
  bool HasAccelerator(char16_t key);

  // 显示其快捷键为|KEY|的子菜单。
  void ActivateAccelerator(char16_t key);

  // 返回根菜单中有多少项。
  int GetItemCount() const;

  // 获取指定屏幕点下的菜单。
  bool GetMenuButtonFromScreenPoint(const gfx::Point& point,
                                    ElectronMenuModel** menu_model,
                                    views::MenuButton** button);

 private:
  // MenuDelegate：：观察者：
  void OnBeforeExecuteCommand() override;
  void OnMenuClosed() override;

  // NativeWindowViewer：
  void OnWindowBlur() override;
  void OnWindowFocus() override;

  // 视图：：AccessiblePaneView：
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool SetPaneFocusAndFocusDefault() override;
  void OnThemeChanged() override;

  // 视图：：FocusChangeListener：
  void OnDidChangeFocus(View* focused_before, View* focused_now) override;

  // 视图：：视图：
  const char* GetClassName() const override;

  void ButtonPressed(int id, const ui::Event& event);

  void RebuildChildren();
  void UpdateViewColors();
  void RefreshColorCache(const ui::NativeTheme* theme);
  View* FindAccelChild(char16_t key);

  SkColor background_color_;
#if defined(OS_LINUX)
  SkColor enabled_color_;
  SkColor disabled_color_;
#endif

  NativeWindow* window_;
  RootView* root_view_;
  ElectronMenuModel* menu_model_ = nullptr;
  bool accelerator_installed_ = false;

  DISALLOW_COPY_AND_ASSIGN(MenuBar);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Views_Menu_BAR_H_
