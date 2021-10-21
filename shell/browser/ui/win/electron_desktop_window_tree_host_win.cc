// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/win/electron_desktop_window_tree_host_win.h"

#include "base/win/windows_version.h"
#include "electron/buildflags/buildflags.h"
#include "shell/browser/ui/views/win_frame_view.h"
#include "ui/base/win/hwnd_metrics.h"
#include "ui/base/win/shell.h"

#if BUILDFLAG(ENABLE_WIN_DARK_MODE_WINDOW_UI)
#include "shell/browser/win/dark_mode.h"
#endif

namespace electron {

ElectronDesktopWindowTreeHostWin::ElectronDesktopWindowTreeHostWin(
    NativeWindowViews* native_window_view,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura)
    : views::DesktopWindowTreeHostWin(native_window_view->widget(),
                                      desktop_native_widget_aura),
      native_window_view_(native_window_view) {}

ElectronDesktopWindowTreeHostWin::~ElectronDesktopWindowTreeHostWin() = default;

bool ElectronDesktopWindowTreeHostWin::PreHandleMSG(UINT message,
                                                    WPARAM w_param,
                                                    LPARAM l_param,
                                                    LRESULT* result) {
#if BUILDFLAG(ENABLE_WIN_DARK_MODE_WINDOW_UI)
  if (message == WM_NCCREATE) {
    HWND const hwnd = GetAcceleratedWidget();
    auto const theme_source =
        ui::NativeTheme::GetInstanceForNativeUi()->theme_source();
    win::SetDarkModeForWindow(hwnd, theme_source);
  }
#endif

  return native_window_view_->PreHandleMSG(message, w_param, l_param, result);
}

bool ElectronDesktopWindowTreeHostWin::ShouldPaintAsActive() const {
  // 告诉Chromium在渲染不活动时使用系统默认行为。
  // 标题栏，否则它会将不活动的标题栏呈现为活动的。
  // 有些案子。
  // 另请参阅https://github.com/electron/electron/issues/24647.。
  return false;
}

bool ElectronDesktopWindowTreeHostWin::HasNativeFrame() const {
  // 因为我们从来不使用Chrome的标题栏实现，所以我们只能说。
  // 我们使用本地标题栏。这将在以下情况下禁用重绘锁定。
  // DWM合成已禁用。
  // 另请参阅https://github.com/electron/electron/issues/1821.。
  return !ui::win::IsAeroGlassEnabled();
}

bool ElectronDesktopWindowTreeHostWin::GetDwmFrameInsetsInPixels(
    gfx::Insets* insets) const {
  // 设置DWMFrameInsets以防止最大化的无框架窗口出血。
  // 连接到其他监视器上。
  if (IsMaximized() && !native_window_view_->has_frame()) {
    // 这相当于调用：
    // DwmExtendFrameIntoClientArea({0，0，0，0})；
    // 
    // 这意味着不要将窗口框架扩展到客户区。就快到了。
    // 一个no-op，但它可以告诉Windows不要将窗口框架扩展为。
    // 比当前工作区大。
    // 
    // 另见：
    // Https://devblogs.microsoft.com/oldnewthing/20150304-00/?p=44543。
    *insets = gfx::Insets();
    return true;
  }
  return false;
}

bool ElectronDesktopWindowTreeHostWin::GetClientAreaInsets(
    gfx::Insets* insets,
    HMONITOR monitor) const {
  // 默认情况下，窗口会将最大化窗口扩展到略大于。
  // 当前工作区，用于无框架窗口，因为标准框架。
  // 删除后，工作区将绘制在当前工作区之外。
  // 
  // 缩进工作区可以修复此行为。
  if (IsMaximized() && !native_window_view_->has_frame()) {
    // 这些嵌套最终将传递给WM_NCCALCSIZE，它接受。
    // _main_monitor的DPI下的指标，而不是当前moniotr。
    // 
    // 请确保您在多个窗口下测试了最大化的无框架窗口。
    // 更改此代码之前使用不同DPI的显示器。
    const int thickness = ::GetSystemMetrics(SM_CXSIZEFRAME) +
                          ::GetSystemMetrics(SM_CXPADDEDBORDER);
    insets->Set(thickness, thickness, thickness, thickness);
    return true;
  }
  return false;
}

}  // 命名空间电子
