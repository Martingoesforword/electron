// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_WIN_ELECTRON_DESKTOP_NATIVE_WIDGET_AURA_H_
#define SHELL_BROWSER_UI_WIN_ELECTRON_DESKTOP_NATIVE_WIDGET_AURA_H_

#include "shell/browser/native_window_views.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"

namespace views {
class DesktopWindowTreeHost;
}

namespace electron {

class ElectronDesktopNativeWidgetAura : public views::DesktopNativeWidgetAura {
 public:
  explicit ElectronDesktopNativeWidgetAura(
      NativeWindowViews* native_window_view);

  // 视图：：DesktopNativeWidgetAura：
  void InitNativeWidget(views::Widget::InitParams params) override;

  // 内部：：NativeWidgetPrivate：
  void Activate() override;

 private:
  void OnWindowActivated(wm::ActivationChangeObserver::ActivationReason reason,
                         aura::Window* gained_active,
                         aura::Window* lost_active) override;

  NativeWindowViews* native_window_view_;

  // 由DesktopNativeWidgetAura所有。
  views::DesktopWindowTreeHost* desktop_window_tree_host_;

  DISALLOW_COPY_AND_ASSIGN(ElectronDesktopNativeWidgetAura);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_WIN_ELECTRON_DESKTOP_NATIVE_WIDGET_AURA_H_
