// 版权所有(C)2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#include "shell/browser/ui/views/electron_views_delegate.h"

#include <memory>

#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/native_widget_aura.h"

#if defined(OS_LINUX)
#include "base/environment.h"
#include "base/nix/xdg_util.h"
#include "ui/views/linux_ui/linux_ui.h"
#endif

namespace {

#if defined(OS_LINUX)
bool IsDesktopEnvironmentUnity() {
  auto env = base::Environment::Create();
  base::nix::DesktopEnvironment desktop_env =
      base::nix::GetDesktopEnvironment(env.get());
  return desktop_env == base::nix::DESKTOP_ENVIRONMENT_UNITY;
}
#endif

}  // 命名空间。

namespace electron {

ViewsDelegate::ViewsDelegate() = default;

ViewsDelegate::~ViewsDelegate() = default;

void ViewsDelegate::SaveWindowPlacement(const views::Widget* window,
                                        const std::string& window_name,
                                        const gfx::Rect& bounds,
                                        ui::WindowShowState show_state) {}

bool ViewsDelegate::GetSavedWindowPlacement(
    const views::Widget* widget,
    const std::string& window_name,
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  return false;
}

void ViewsDelegate::NotifyMenuItemFocused(const std::u16string& menu_name,
                                          const std::u16string& menu_item_name,
                                          int item_index,
                                          int item_count,
                                          bool has_submenu) {}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
gfx::ImageSkia* ViewsDelegate::GetDefaultWindowIcon() const {
  return nullptr;
}
#endif

std::unique_ptr<views::NonClientFrameView>
ViewsDelegate::CreateDefaultNonClientFrameView(views::Widget* widget) {
  return nullptr;
}

void ViewsDelegate::AddRef() {}

void ViewsDelegate::ReleaseRef() {}

void ViewsDelegate::OnBeforeWidgetInit(
    views::Widget::InitParams* params,
    views::internal::NativeWidgetDelegate* delegate) {
  // 如果我们已经有了Native_Widget，我们就不必尝试来了。
  // 加一个。
  if (params->native_widget)
    return;

  if (params->parent && params->type != views::Widget::InitParams::TYPE_MENU &&
      params->type != views::Widget::InitParams::TYPE_TOOLTIP) {
    params->native_widget = new views::NativeWidgetAura(delegate);
  } else {
    params->native_widget = new views::DesktopNativeWidgetAura(delegate);
  }
}

bool ViewsDelegate::WindowManagerProvidesTitleBar(bool maximized) {
#if defined(OS_LINUX)
  // 在Ubuntu Unity上，系统总是提供最大化的标题栏。
  // 窗户。
  if (!maximized)
    return false;
  static bool is_desktop_environment_unity = IsDesktopEnvironmentUnity();
  return is_desktop_environment_unity;
#else
  return false;
#endif
}

}  // 命名空间电子
