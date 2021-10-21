// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_power_monitor.h"

#include <windows.h>
#include <wtsapi32.h>

#include "base/win/windows_types.h"
#include "base/win/wrapped_window_proc.h"
#include "content/public/browser/browser_task_traits.h"
#include "ui/base/win/shell.h"
#include "ui/gfx/win/hwnd_util.h"

namespace electron {

namespace {

const wchar_t kPowerMonitorWindowClass[] = L"Electron_PowerMonitorHostWindow";

}  // 命名空间。

namespace api {

void PowerMonitor::InitPlatformSpecificMonitors() {
  WNDCLASSEX window_class;
  base::win::InitializeWindowClass(
      kPowerMonitorWindowClass,
      &base::win::WrappedWindowProc<PowerMonitor::WndProcStatic>, 0, 0, 0, NULL,
      NULL, NULL, NULL, NULL, &window_class);
  instance_ = window_class.hInstance;
  atom_ = RegisterClassEx(&window_class);

  // 为接收广播消息创建离屏窗口。
  // 会话锁定和解锁事件。
  window_ = CreateWindow(MAKEINTATOM(atom_), 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0,
                         instance_, 0);
  gfx::CheckWindowCreated(window_, ::GetLastError());
  gfx::SetWindowUserData(window_, this);

  // 通知窗口我们希望收到会话事件通知。
  WTSRegisterSessionNotification(window_, NOTIFY_FOR_THIS_SESSION);

  // 对于Windows8和更高版本，增加了一个新的“连接待机”模式，
  // 我们必须明确注册它的通知。
  auto RegisterSuspendResumeNotification =
      reinterpret_cast<decltype(&::RegisterSuspendResumeNotification)>(
          GetProcAddress(GetModuleHandle(L"user32.dll"),
                         "RegisterSuspendResumeNotification"));

  if (RegisterSuspendResumeNotification) {
    RegisterSuspendResumeNotification(static_cast<HANDLE>(window_),
                                      DEVICE_NOTIFY_WINDOW_HANDLE);
  }
}

LRESULT CALLBACK PowerMonitor::WndProcStatic(HWND hwnd,
                                             UINT message,
                                             WPARAM wparam,
                                             LPARAM lparam) {
  auto* msg_wnd =
      reinterpret_cast<PowerMonitor*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg_wnd)
    return msg_wnd->WndProc(hwnd, message, wparam, lparam);
  else
    return ::DefWindowProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK PowerMonitor::WndProc(HWND hwnd,
                                       UINT message,
                                       WPARAM wparam,
                                       LPARAM lparam) {
  if (message == WM_WTSSESSION_CHANGE) {
    bool should_treat_as_current_session = true;
    DWORD current_session_id = 0;
    if (!::ProcessIdToSessionId(::GetCurrentProcessId(), &current_session_id)) {
      LOG(ERROR) << "ProcessIdToSessionId failed, assuming current session";
    } else {
      should_treat_as_current_session =
          (static_cast<DWORD>(lparam) == current_session_id);
    }
    if (should_treat_as_current_session) {
      if (wparam == WTS_SESSION_LOCK) {
        // 取消保留是可以的，因为此对象是永久固定的。
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce([](PowerMonitor* pm) { pm->Emit("lock-screen"); },
                           base::Unretained(this)));
      } else if (wparam == WTS_SESSION_UNLOCK) {
        content::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE,
            base::BindOnce([](PowerMonitor* pm) { pm->Emit("unlock-screen"); },
                           base::Unretained(this)));
      }
    }
  } else if (message == WM_POWERBROADCAST) {
    if (wparam == PBT_APMRESUMEAUTOMATIC) {
      content::GetUIThreadTaskRunner({})->PostTask(
          FROM_HERE,
          base::BindOnce([](PowerMonitor* pm) { pm->Emit("resume"); },
                         base::Unretained(this)));
    } else if (wparam == PBT_APMSUSPEND) {
      content::GetUIThreadTaskRunner({})->PostTask(
          FROM_HERE,
          base::BindOnce([](PowerMonitor* pm) { pm->Emit("suspend"); },
                         base::Unretained(this)));
    }
  }
  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

}  // 命名空间API。

}  // 命名空间电子
