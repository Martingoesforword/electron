// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_ROOT_VIEW_H_
#define SHELL_BROWSER_UI_VIEWS_ROOT_VIEW_H_

#include <memory>

#include "shell/browser/ui/accelerator_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/view.h"
#include "ui/views/view_tracker.h"

namespace content {
struct NativeWebKeyboardEvent;
}

namespace electron {

class ElectronMenuModel;
class MenuBar;
class NativeWindow;

class RootView : public views::View {
 public:
  explicit RootView(NativeWindow* window);
  ~RootView() override;

  void SetMenu(ElectronMenuModel* menu_model);
  bool HasMenu() const;
  int GetMenuBarHeight() const;
  void SetAutoHideMenuBar(bool auto_hide);
  bool IsMenuBarAutoHide() const;
  void SetMenuBarVisibility(bool visible);
  bool IsMenuBarVisible() const;
  void HandleKeyEvent(const content::NativeWebKeyboardEvent& event);
  void ResetAltState();
  void RestoreFocus();
  // 注册/取消注册菜单型号支持的快捷键。
  void RegisterAcceleratorsWithFocusManager(ElectronMenuModel* menu_model);
  void UnregisterAcceleratorsWithFocusManager();

  // 视图：：视图：
  void Layout() override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;

 private:
  // 父窗口，弱引用。
  NativeWindow* window_;

  // 菜单栏。
  std::unique_ptr<MenuBar> menu_bar_;
  bool menu_bar_autohide_ = false;
  bool menu_bar_visible_ = false;
  bool menu_bar_alt_pressed_ = false;

  // 从快捷键映射到菜单项的命令ID。
  accelerator_util::AcceleratorTable accelerator_table_;

  std::unique_ptr<views::ViewTracker> last_focused_view_tracker_;

  DISALLOW_COPY_AND_ASSIGN(RootView);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Views_ROOT_VIEW_H_
