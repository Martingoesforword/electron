// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/win/electron_desktop_native_widget_aura.h"

#include <utility>

#include "shell/browser/ui/win/electron_desktop_window_tree_host_win.h"
#include "ui/views/corewm/tooltip_controller.h"
#include "ui/wm/public/tooltip_client.h"

namespace electron {

ElectronDesktopNativeWidgetAura::ElectronDesktopNativeWidgetAura(
    NativeWindowViews* native_window_view)
    : views::DesktopNativeWidgetAura(native_window_view->widget()),
      native_window_view_(native_window_view) {
  GetNativeWindow()->SetName("ElectronDesktopNativeWidgetAura");
  // 这是为了启用OnWindowActified的覆盖。
  wm::SetActivationChangeObserver(GetNativeWindow(), this);
}

void ElectronDesktopNativeWidgetAura::InitNativeWidget(
    views::Widget::InitParams params) {
  desktop_window_tree_host_ = new ElectronDesktopWindowTreeHostWin(
      native_window_view_,
      static_cast<views::DesktopNativeWidgetAura*>(params.native_widget));
  params.desktop_window_tree_host = desktop_window_tree_host_;
  views::DesktopNativeWidgetAura::InitNativeWidget(std::move(params));
}

void ElectronDesktopNativeWidgetAura::Activate() {
  // 激活可使聚焦窗口变得模糊，因此仅。
  // 当激活的窗口可见时调用。这会阻止。
  // 隐藏窗口，避免在创建时模糊聚焦窗口。
  if (IsVisible())
    views::DesktopNativeWidgetAura::Activate();
}

void ElectronDesktopNativeWidgetAura::OnWindowActivated(
    wm::ActivationChangeObserver::ActivationReason reason,
    aura::Window* gained_active,
    aura::Window* lost_active) {
  views::DesktopNativeWidgetAura::OnWindowActivated(reason, gained_active,
                                                    lost_active);
  if (lost_active != nullptr) {
    auto* tooltip_controller = static_cast<views::corewm::TooltipController*>(
        wm::GetTooltipClient(lost_active->GetRootWindow()));

    // 这将导致在停用窗口时隐藏工具提示，
    // 应该是这样的。
    // TODO(Brenca)：修复铬问题后删除此修复。
    // Crbug.com/724538。
    if (tooltip_controller != nullptr)
      tooltip_controller->OnCancelMode(nullptr);
  }
}

}  // 命名空间电子
