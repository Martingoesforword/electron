// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/win/notify_icon_host.h"

#include <commctrl.h>
#include <winuser.h>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/stl_util.h"
#include "base/win/win_util.h"
#include "base/win/windows_types.h"
#include "base/win/wrapped_window_proc.h"
#include "content/public/browser/browser_task_traits.h"
#include "shell/browser/ui/win/notify_icon.h"
#include "ui/events/event_constants.h"
#include "ui/events/win/system_event_state_lookup.h"
#include "ui/gfx/win/hwnd_util.h"

namespace electron {

namespace {

const UINT kNotifyIconMessage = WM_APP + 1;

// |kBaseIconId|为2，避免与硬编码id为1的插件冲突。
const UINT kBaseIconId = 2;

const wchar_t kNotifyIconHostWindowClass[] = L"Electron_NotifyIconHostWindow";

bool IsWinPressed() {
  return ((::GetKeyState(VK_LWIN) & 0x8000) == 0x8000) ||
         ((::GetKeyState(VK_RWIN) & 0x8000) == 0x8000);
}

int GetKeyboardModifiers() {
  int modifiers = ui::EF_NONE;
  if (ui::win::IsShiftPressed())
    modifiers |= ui::EF_SHIFT_DOWN;
  if (ui::win::IsCtrlPressed())
    modifiers |= ui::EF_CONTROL_DOWN;
  if (ui::win::IsAltPressed())
    modifiers |= ui::EF_ALT_DOWN;
  if (IsWinPressed())
    modifiers |= ui::EF_COMMAND_DOWN;
  return modifiers;
}

}  // 命名空间。

NotifyIconHost::NotifyIconHost() {
  // 注册我们的窗口类。
  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kNotifyIconHostWindowClass,
      &base::win::WrappedWindowProc<NotifyIconHost::WndProcStatic>, 0, 0, 0,
      NULL, NULL, NULL, NULL, NULL, &window_class);
  instance_ = window_class.hInstance;
  atom_ = RegisterClassEx(&window_class);
  CHECK(atom_);

  // 如果在启动后重新创建任务栏，则必须重新构建所有。
  // 我们的偶像。
  taskbar_created_message_ = RegisterWindowMessage(TEXT("TaskbarCreated"));

  // 创建一个离屏窗口，用于处理状态图标的消息。我们。
  // 创建隐藏的WS_POPUP窗口而不是HWND_MESSAGE窗口，因为。
  // 只有顶级窗口(如弹出窗口)才能接收广播消息，如。
  // “任务栏已创建”。
  window_ = CreateWindow(MAKEINTATOM(atom_), 0, WS_POPUP, 0, 0, 0, 0, 0, 0,
                         instance_, 0);
  gfx::CheckWindowCreated(window_, ::GetLastError());
  gfx::SetWindowUserData(window_, this);
}

NotifyIconHost::~NotifyIconHost() {
  if (window_)
    DestroyWindow(window_);

  if (atom_)
    UnregisterClass(MAKEINTATOM(atom_), instance_);

  for (NotifyIcon* ptr : notify_icons_)
    delete ptr;
}

NotifyIcon* NotifyIconHost::CreateNotifyIcon(absl::optional<UUID> guid) {
  if (guid.has_value()) {
    for (NotifyIcons::const_iterator i(notify_icons_.begin());
         i != notify_icons_.end(); ++i) {
      auto* current_win_icon = static_cast<NotifyIcon*>(*i);
      if (current_win_icon->guid() == guid.value()) {
        LOG(WARNING)
            << "Guid already in use. Existing tray entry will be replaced.";
      }
    }
  }

  auto* notify_icon =
      new NotifyIcon(this, NextIconId(), window_, kNotifyIconMessage,
                     guid.has_value() ? guid.value() : GUID_DEFAULT);

  notify_icons_.push_back(notify_icon);
  return notify_icon;
}

void NotifyIconHost::Remove(NotifyIcon* icon) {
  NotifyIcons::iterator i(
      std::find(notify_icons_.begin(), notify_icons_.end(), icon));

  if (i == notify_icons_.end()) {
    NOTREACHED();
    return;
  }

  notify_icons_.erase(i);
}

LRESULT CALLBACK NotifyIconHost::WndProcStatic(HWND hwnd,
                                               UINT message,
                                               WPARAM wparam,
                                               LPARAM lparam) {
  auto* msg_wnd =
      reinterpret_cast<NotifyIconHost*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg_wnd)
    return msg_wnd->WndProc(hwnd, message, wparam, lparam);
  else
    return ::DefWindowProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK NotifyIconHost::WndProc(HWND hwnd,
                                         UINT message,
                                         WPARAM wparam,
                                         LPARAM lparam) {
  if (message == taskbar_created_message_) {
    // 我们需要重置所有图标，因为任务栏消失了。
    for (NotifyIcons::const_iterator i(notify_icons_.begin());
         i != notify_icons_.end(); ++i) {
      auto* win_icon = static_cast<NotifyIcon*>(*i);
      win_icon->ResetIcon();
    }
    return TRUE;
  } else if (message == kNotifyIconMessage) {
    NotifyIcon* win_icon = NULL;

    // 查找选定的状态图标。
    for (NotifyIcons::const_iterator i(notify_icons_.begin());
         i != notify_icons_.end(); ++i) {
      auto* current_win_icon = static_cast<NotifyIcon*>(*i);
      if (current_win_icon->icon_id() == wparam) {
        win_icon = current_win_icon;
        break;
      }
    }

    // 可以使用过时图标调用此过程。
    // 身份证。那样的话，我们应该在处理任何。
    // 行为。
    if (!win_icon)
      return TRUE;

    // 我们在这里对NotifyIcons使用WeakPtr工厂，因此。
    // 如果NotifyIcon获得。
    // 垃圾收集。当托盘到达时，就会发生这种情况。
    // GC，下面的气球事件将不会发射。
    base::WeakPtr<NotifyIcon> win_icon_weak = win_icon->GetWeakPtr();

    switch (lparam) {
      case NIN_BALLOONSHOW:
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce(&NotifyIcon::NotifyBalloonShow, win_icon_weak));
        return TRUE;

      case NIN_BALLOONUSERCLICK:
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce(&NotifyIcon::NotifyBalloonClicked, win_icon_weak));
        return TRUE;

      case NIN_BALLOONTIMEOUT:
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce(&NotifyIcon::NotifyBalloonClosed, win_icon_weak));
        return TRUE;

      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDBLCLK:
      case WM_CONTEXTMENU:
        // 浏览我们的图标，找到被点击的图标，并调用其。
        // HandleClickEvent()方法。
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce(
                &NotifyIcon::HandleClickEvent, win_icon_weak,
                GetKeyboardModifiers(),
                (lparam == WM_LBUTTONDOWN || lparam == WM_LBUTTONDBLCLK),
                (lparam == WM_LBUTTONDBLCLK || lparam == WM_RBUTTONDBLCLK)));

        return TRUE;

      case WM_MOUSEMOVE:
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE, base::BindOnce(&NotifyIcon::HandleMouseMoveEvent,
                                      win_icon_weak, GetKeyboardModifiers()));
        return TRUE;
    }
  }
  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

UINT NotifyIconHost::NextIconId() {
  UINT icon_id = next_icon_id_++;
  return kBaseIconId + icon_id;
}

}  // 命名空间电子
