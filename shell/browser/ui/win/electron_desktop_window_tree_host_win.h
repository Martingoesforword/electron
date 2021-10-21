// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_WIN_ELECTRON_DESKTOP_WINDOW_TREE_HOST_WIN_H_
#define SHELL_BROWSER_UI_WIN_ELECTRON_DESKTOP_WINDOW_TREE_HOST_WIN_H_

#include <windows.h>

#include "shell/browser/native_window_views.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_win.h"

namespace electron {

class ElectronDesktopWindowTreeHostWin
    : public views::DesktopWindowTreeHostWin {
 public:
  ElectronDesktopWindowTreeHostWin(
      NativeWindowViews* native_window_view,
      views::DesktopNativeWidgetAura* desktop_native_widget_aura);
  ~ElectronDesktopWindowTreeHostWin() override;

 protected:
  bool PreHandleMSG(UINT message,
                    WPARAM w_param,
                    LPARAM l_param,
                    LRESULT* result) override;
  bool ShouldPaintAsActive() const override;
  bool HasNativeFrame() const override;
  bool GetDwmFrameInsetsInPixels(gfx::Insets* insets) const override;
  bool GetClientAreaInsets(gfx::Insets* insets,
                           HMONITOR monitor) const override;

 private:
  NativeWindowViews* native_window_view_;  // 弱裁判。

  DISALLOW_COPY_AND_ASSIGN(ElectronDesktopWindowTreeHostWin);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_WIN_ELECTRON_DESKTOP_WINDOW_TREE_HOST_WIN_H_
