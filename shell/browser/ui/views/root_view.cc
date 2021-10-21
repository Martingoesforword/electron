// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/views/root_view.h"

#include <memory>

#include "content/public/browser/native_web_keyboard_event.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/views/menu_bar.h"

namespace electron {

namespace {

// 菜单栏高度(以像素为单位)。
#if defined(OS_WIN)
const int kMenuBarHeight = 20;
#else
const int kMenuBarHeight = 25;
#endif

bool IsAltKey(const content::NativeWebKeyboardEvent& event) {
  return event.windows_key_code == ui::VKEY_MENU;
}

bool IsAltModifier(const content::NativeWebKeyboardEvent& event) {
  typedef content::NativeWebKeyboardEvent::Modifiers Modifiers;
  int modifiers = event.GetModifiers();
  modifiers &= ~Modifiers::kNumLockOn;
  modifiers &= ~Modifiers::kCapsLockOn;
  return (modifiers == Modifiers::kAltKey) ||
         (modifiers == (Modifiers::kAltKey | Modifiers::kIsLeft)) ||
         (modifiers == (Modifiers::kAltKey | Modifiers::kIsRight));
}

}  // 命名空间。

RootView::RootView(NativeWindow* window)
    : window_(window),
      last_focused_view_tracker_(std::make_unique<views::ViewTracker>()) {
  set_owned_by_client();
}

RootView::~RootView() = default;

void RootView::SetMenu(ElectronMenuModel* menu_model) {
  if (menu_model == nullptr) {
    // 移除加速器。
    UnregisterAcceleratorsWithFocusManager();
    // 和菜单栏。
    SetMenuBarVisibility(false);
    menu_bar_.reset();
    return;
  }

  RegisterAcceleratorsWithFocusManager(menu_model);

  // 不在无框架窗口中显示菜单栏。
  if (!window_->has_frame())
    return;

  if (!menu_bar_) {
    menu_bar_ = std::make_unique<MenuBar>(window_, this);
    menu_bar_->set_owned_by_client();
    if (!menu_bar_autohide_)
      SetMenuBarVisibility(true);
  }

  menu_bar_->SetMenu(menu_model);
  Layout();
}

bool RootView::HasMenu() const {
  return !!menu_bar_;
}

int RootView::GetMenuBarHeight() const {
  return kMenuBarHeight;
}

void RootView::SetAutoHideMenuBar(bool auto_hide) {
  menu_bar_autohide_ = auto_hide;
}

bool RootView::IsMenuBarAutoHide() const {
  return menu_bar_autohide_;
}

void RootView::SetMenuBarVisibility(bool visible) {
  if (!window_->content_view() || !menu_bar_ || menu_bar_visible_ == visible)
    return;

  menu_bar_visible_ = visible;
  if (visible) {
    DCHECK_EQ(children().size(), 1ul);
    AddChildView(menu_bar_.get());
  } else {
    DCHECK_EQ(children().size(), 2ul);
    RemoveChildView(menu_bar_.get());
  }

  Layout();
}

bool RootView::IsMenuBarVisible() const {
  return menu_bar_visible_;
}

void RootView::HandleKeyEvent(const content::NativeWebKeyboardEvent& event) {
  if (!menu_bar_)
    return;

  // 按下“Alt”时显示快捷键。
  if (menu_bar_visible_ && IsAltKey(event))
    menu_bar_->SetAcceleratorVisibility(
        event.GetType() == blink::WebInputEvent::Type::kRawKeyDown);

  // 按下Alt+键时显示子菜单。
  if (event.GetType() == blink::WebInputEvent::Type::kRawKeyDown &&
      event.windows_key_code >= ui::VKEY_A &&
      event.windows_key_code <= ui::VKEY_Z && IsAltModifier(event) &&
      menu_bar_->HasAccelerator(event.windows_key_code)) {
    if (!menu_bar_visible_) {
      SetMenuBarVisibility(true);

      View* focused_view = GetFocusManager()->GetFocusedView();
      last_focused_view_tracker_->SetView(focused_view);
      menu_bar_->RequestFocus();
    }

    menu_bar_->ActivateAccelerator(event.windows_key_code);
    return;
  }

  // 仅在释放单个Alt键时切换菜单栏。
  if (event.GetType() == blink::WebInputEvent::Type::kRawKeyDown &&
      IsAltKey(event)) {
    // 当按下单个Alt时：
    menu_bar_alt_pressed_ = true;
  } else if (event.GetType() == blink::WebInputEvent::Type::kKeyUp &&
             IsAltKey(event) && menu_bar_alt_pressed_) {
    // 在按下Alt后立即释放单个Alt时：
    menu_bar_alt_pressed_ = false;
    if (menu_bar_autohide_)
      SetMenuBarVisibility(!menu_bar_visible_);

    View* focused_view = GetFocusManager()->GetFocusedView();
    last_focused_view_tracker_->SetView(focused_view);
    if (menu_bar_visible_) {
      menu_bar_->RequestFocus();
      // 当菜单栏被聚焦时显示快捷键。
      menu_bar_->SetAcceleratorVisibility(true);
    }
  } else {
    // 当按下/松开除单个Alt键以外的任何其他键时：
    menu_bar_alt_pressed_ = false;
  }
}

void RootView::RestoreFocus() {
  View* last_focused_view = last_focused_view_tracker_->view();
  if (last_focused_view) {
    GetFocusManager()->SetFocusedViewWithReason(
        last_focused_view,
        views::FocusManager::FocusChangeReason::kFocusRestore);
  }
  if (menu_bar_autohide_)
    SetMenuBarVisibility(false);
}

void RootView::ResetAltState() {
  menu_bar_alt_pressed_ = false;
}

void RootView::Layout() {
  if (!window_->content_view())  // 还没准备好。
    return;

  const auto menu_bar_bounds =
      menu_bar_visible_ ? gfx::Rect(0, 0, size().width(), kMenuBarHeight)
                        : gfx::Rect();
  if (menu_bar_)
    menu_bar_->SetBoundsRect(menu_bar_bounds);

  window_->content_view()->SetBoundsRect(
      gfx::Rect(0, menu_bar_visible_ ? menu_bar_bounds.bottom() : 0,
                size().width(), size().height() - menu_bar_bounds.height()));
}

gfx::Size RootView::GetMinimumSize() const {
  return window_->GetMinimumSize();
}

gfx::Size RootView::GetMaximumSize() const {
  return window_->GetMaximumSize();
}

bool RootView::AcceleratorPressed(const ui::Accelerator& accelerator) {
  return accelerator_util::TriggerAcceleratorTableCommand(&accelerator_table_,
                                                          accelerator);
}

void RootView::RegisterAcceleratorsWithFocusManager(
    ElectronMenuModel* menu_model) {
  if (!menu_model)
    return;
  // 清除以前的加速器。
  UnregisterAcceleratorsWithFocusManager();

  views::FocusManager* focus_manager = GetFocusManager();
  // 向焦点管理器注册加速器。
  accelerator_util::GenerateAcceleratorTable(&accelerator_table_, menu_model);
  for (const auto& iter : accelerator_table_) {
    focus_manager->RegisterAccelerator(
        iter.first, ui::AcceleratorManager::kNormalPriority, this);
  }
}

void RootView::UnregisterAcceleratorsWithFocusManager() {
  views::FocusManager* focus_manager = GetFocusManager();
  accelerator_table_.clear();
  focus_manager->UnregisterAccelerators(this);
}

}  // 命名空间电子
