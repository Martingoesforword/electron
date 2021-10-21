// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include <dwmapi.h>
#include <shellapi.h>

#include "content/public/browser/browser_accessibility_state.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/views/root_view.h"
#include "shell/common/electron_constants.h"
#include "ui/base/win/accessibility_misc_utils.h"
#include "ui/display/display.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/resize_utils.h"
#include "ui/views/widget/native_widget_private.h"

// 必须包含在其他Windows标头之后。
#include <UIAutomationCoreApi.h>

namespace electron {

namespace {

// 将Win32 WM_APPCOMMANDS转换为字符串。
const char* AppCommandToString(int command_id) {
  switch (command_id) {
    case APPCOMMAND_BROWSER_BACKWARD:
      return kBrowserBackward;
    case APPCOMMAND_BROWSER_FORWARD:
      return kBrowserForward;
    case APPCOMMAND_BROWSER_REFRESH:
      return "browser-refresh";
    case APPCOMMAND_BROWSER_STOP:
      return "browser-stop";
    case APPCOMMAND_BROWSER_SEARCH:
      return "browser-search";
    case APPCOMMAND_BROWSER_FAVORITES:
      return "browser-favorites";
    case APPCOMMAND_BROWSER_HOME:
      return "browser-home";
    case APPCOMMAND_VOLUME_MUTE:
      return "volume-mute";
    case APPCOMMAND_VOLUME_DOWN:
      return "volume-down";
    case APPCOMMAND_VOLUME_UP:
      return "volume-up";
    case APPCOMMAND_MEDIA_NEXTTRACK:
      return "media-nexttrack";
    case APPCOMMAND_MEDIA_PREVIOUSTRACK:
      return "media-previoustrack";
    case APPCOMMAND_MEDIA_STOP:
      return "media-stop";
    case APPCOMMAND_MEDIA_PLAY_PAUSE:
      return "media-play-pause";
    case APPCOMMAND_LAUNCH_MAIL:
      return "launch-mail";
    case APPCOMMAND_LAUNCH_MEDIA_SELECT:
      return "launch-media-select";
    case APPCOMMAND_LAUNCH_APP1:
      return "launch-app1";
    case APPCOMMAND_LAUNCH_APP2:
      return "launch-app2";
    case APPCOMMAND_BASS_DOWN:
      return "bass-down";
    case APPCOMMAND_BASS_BOOST:
      return "bass-boost";
    case APPCOMMAND_BASS_UP:
      return "bass-up";
    case APPCOMMAND_TREBLE_DOWN:
      return "treble-down";
    case APPCOMMAND_TREBLE_UP:
      return "treble-up";
    case APPCOMMAND_MICROPHONE_VOLUME_MUTE:
      return "microphone-volume-mute";
    case APPCOMMAND_MICROPHONE_VOLUME_DOWN:
      return "microphone-volume-down";
    case APPCOMMAND_MICROPHONE_VOLUME_UP:
      return "microphone-volume-up";
    case APPCOMMAND_HELP:
      return "help";
    case APPCOMMAND_FIND:
      return "find";
    case APPCOMMAND_NEW:
      return "new";
    case APPCOMMAND_OPEN:
      return "open";
    case APPCOMMAND_CLOSE:
      return "close";
    case APPCOMMAND_SAVE:
      return "save";
    case APPCOMMAND_PRINT:
      return "print";
    case APPCOMMAND_UNDO:
      return "undo";
    case APPCOMMAND_REDO:
      return "redo";
    case APPCOMMAND_COPY:
      return "copy";
    case APPCOMMAND_CUT:
      return "cut";
    case APPCOMMAND_PASTE:
      return "paste";
    case APPCOMMAND_REPLY_TO_MAIL:
      return "reply-to-mail";
    case APPCOMMAND_FORWARD_MAIL:
      return "forward-mail";
    case APPCOMMAND_SEND_MAIL:
      return "send-mail";
    case APPCOMMAND_SPELL_CHECK:
      return "spell-check";
    case APPCOMMAND_MIC_ON_OFF_TOGGLE:
      return "mic-on-off-toggle";
    case APPCOMMAND_CORRECTION_LIST:
      return "correction-list";
    case APPCOMMAND_MEDIA_PLAY:
      return "media-play";
    case APPCOMMAND_MEDIA_PAUSE:
      return "media-pause";
    case APPCOMMAND_MEDIA_RECORD:
      return "media-record";
    case APPCOMMAND_MEDIA_FAST_FORWARD:
      return "media-fast-forward";
    case APPCOMMAND_MEDIA_REWIND:
      return "media-rewind";
    case APPCOMMAND_MEDIA_CHANNEL_UP:
      return "media-channel-up";
    case APPCOMMAND_MEDIA_CHANNEL_DOWN:
      return "media-channel-down";
    case APPCOMMAND_DELETE:
      return "delete";
    case APPCOMMAND_DICTATE_OR_COMMAND_CONTROL_TOGGLE:
      return "dictate-or-command-control-toggle";
    default:
      return "unknown";
  }
}

// 从ui/views/win/hwnd_message_handler.cc复制。
gfx::ResizeEdge GetWindowResizeEdge(WPARAM param) {
  switch (param) {
    case WMSZ_BOTTOM:
      return gfx::ResizeEdge::kBottom;
    case WMSZ_TOP:
      return gfx::ResizeEdge::kTop;
    case WMSZ_LEFT:
      return gfx::ResizeEdge::kLeft;
    case WMSZ_RIGHT:
      return gfx::ResizeEdge::kRight;
    case WMSZ_TOPLEFT:
      return gfx::ResizeEdge::kTopLeft;
    case WMSZ_TOPRIGHT:
      return gfx::ResizeEdge::kTopRight;
    case WMSZ_BOTTOMLEFT:
      return gfx::ResizeEdge::kBottomLeft;
    case WMSZ_BOTTOMRIGHT:
      return gfx::ResizeEdge::kBottomRight;
    default:
      return gfx::ResizeEdge::kBottomRight;
  }
}

bool IsScreenReaderActive() {
  UINT screenReader = 0;
  SystemParametersInfo(SPI_GETSCREENREADER, 0, &screenReader, 0);
  return screenReader && UiaClientsAreListening();
}

}  // 命名空间。

std::set<NativeWindowViews*> NativeWindowViews::forwarding_windows_;
HHOOK NativeWindowViews::mouse_hook_ = NULL;

void NativeWindowViews::Maximize() {
  // 仅当窗口不是透明样式时才使用最大化()。
  if (!transparent()) {
    if (IsVisible())
      widget()->Maximize();
    else
      widget()->native_widget_private()->Show(ui::SHOW_STATE_MAXIMIZED,
                                              gfx::Rect());
    return;
  } else {
    restore_bounds_ = GetBounds();
    auto display = display::Screen::GetScreen()->GetDisplayNearestWindow(
        GetNativeWindow());
    SetBounds(display.work_area(), false);
  }
}

bool NativeWindowViews::ExecuteWindowsCommand(int command_id) {
  std::string command = AppCommandToString(command_id);
  NotifyWindowExecuteAppCommand(command);

  return false;
}

bool NativeWindowViews::PreHandleMSG(UINT message,
                                     WPARAM w_param,
                                     LPARAM l_param,
                                     LRESULT* result) {
  NotifyWindowMessage(message, w_param, l_param);

  // 调用SetWindowPlacement时避免副作用。
  if (is_setting_window_placement_) {
    // 让Chromium处理WM_NCCALCSIZE消息，否则窗口大小。
    // 那就大错特错了。
    // 有关更多信息，请参见https://github.com/electron/electron/issues/22393。
    if (message == WM_NCCALCSIZE)
      return false;
    // 否则用缺省流程处理报文，
    *result = DefWindowProc(GetAcceleratedWidget(), message, w_param, l_param);
    // 告诉Chromium忽略这条消息。
    return true;
  }

  switch (message) {
    // 屏幕阅读器发送WM_GETOBJECT以获取可访问性。
    // 对象，所以借此机会将铬推向可访问的。
    // 模式(如果还没有的话)，总是说我们没有处理消息。
    // 因为我们仍然希望Chromium处理返回实际的。
    // 辅助功能对象。
    case WM_GETOBJECT: {
      if (checked_for_a11y_support_)
        return false;

      const DWORD obj_id = static_cast<DWORD>(l_param);

      if (obj_id != static_cast<DWORD>(OBJID_CLIENT)) {
        return false;
      }

      if (!IsScreenReaderActive()) {
        return false;
      }

      checked_for_a11y_support_ = true;

      auto* const axState = content::BrowserAccessibilityState::GetInstance();
      if (axState && !axState->IsAccessibleBrowser()) {
        axState->OnScreenReaderDetected();
        Browser::Get()->OnAccessibilitySupportChanged();
      }

      return false;
    }
    case WM_GETMINMAXINFO: {
      WINDOWPLACEMENT wp;
      wp.length = sizeof(WINDOWPLACEMENT);

      // 我们这样做是为了解决Windows错误，其中最小化的窗口。
      // 会报告说最接近它的显示器不是它原来的那个。
      // 前情提要(不过是最左边的那个)。我们恢复了阵地。
      // 在恢复操作期间，铬可以。
      // 使用正确的显示来计算要使用的比例因子。
      if (!last_normal_placement_bounds_.IsEmpty() &&
          (IsVisible() || IsMinimized()) &&
          GetWindowPlacement(GetAcceleratedWidget(), &wp)) {
        wp.rcNormalPosition = last_normal_placement_bounds_.ToRECT();

        // 当调用SetWindowPlacement时，Chromium将执行窗口消息。
        // 正在处理。但由于我们已经在PreHandleMSG中，这将导致。
        // 在某些情况下会在Chromium中崩溃。
        // 
        // 我们通过阻止Chromium处理窗口来解决崩溃问题。
        // 消息，直到SetWindowPlacement调用完成。
        // 
        // 有关更多信息，请参见https://github.com/electron/electron/issues/21614。
        is_setting_window_placement_ = true;
        SetWindowPlacement(GetAcceleratedWidget(), &wp);
        is_setting_window_placement_ = false;

        last_normal_placement_bounds_ = gfx::Rect();
      }

      return false;
    }
    case WM_COMMAND:
      // 处理拇指按钮点击消息。
      if (HIWORD(w_param) == THBN_CLICKED)
        return taskbar_host_.HandleThumbarButtonEvent(LOWORD(w_param));
      return false;
    case WM_SIZING: {
      is_resizing_ = true;
      bool prevent_default = false;
      gfx::Rect bounds = gfx::Rect(*reinterpret_cast<RECT*>(l_param));
      HWND hwnd = GetAcceleratedWidget();
      gfx::Rect dpi_bounds = ScreenToDIPRect(hwnd, bounds);
      NotifyWindowWillResize(dpi_bounds, GetWindowResizeEdge(w_param),
                             &prevent_default);
      if (prevent_default) {
        ::GetWindowRect(hwnd, reinterpret_cast<RECT*>(l_param));
        return true;  // 通知Windows已处理大小调整。
      }
      return false;
    }
    case WM_SIZE: {
      // 处理窗口状态更改。
      HandleSizeEvent(w_param, l_param);
      return false;
    }
    case WM_EXITSIZEMOVE: {
      if (is_resizing_) {
        NotifyWindowResized();
        is_resizing_ = false;
      }
      if (is_moving_) {
        NotifyWindowMoved();
        is_moving_ = false;
      }
      return false;
    }
    case WM_MOVING: {
      is_moving_ = true;
      bool prevent_default = false;
      gfx::Rect bounds = gfx::Rect(*reinterpret_cast<RECT*>(l_param));
      HWND hwnd = GetAcceleratedWidget();
      gfx::Rect dpi_bounds = ScreenToDIPRect(hwnd, bounds);
      NotifyWindowWillMove(dpi_bounds, &prevent_default);
      if (!movable_ || prevent_default) {
        ::GetWindowRect(hwnd, reinterpret_cast<RECT*>(l_param));
        return true;  // 通知Windows移动已处理。如果不是真的，
                      // 可以使用以下工具移动无边框窗口。
                      // -webkit-app-region：拖动元素。
      }
      return false;
    }
    case WM_ENDSESSION: {
      if (w_param) {
        NotifyWindowEndSession();
      }
      return false;
    }
    case WM_PARENTNOTIFY: {
      if (LOWORD(w_param) == WM_CREATE) {
        // 由于有关遗留驱动程序和其他东西的原因，
        // 匹配Chromium在内部创建和使用的工作区。
        // 这在转发鼠标消息时使用。我们只缓存第一个。
        // 发生(Webview窗口)，因为开发工具也会导致这种情况。
        // 要发送的消息。
        if (!legacy_window_) {
          legacy_window_ = reinterpret_cast<HWND>(l_param);
        }
      }
      return false;
    }
    case WM_CONTEXTMENU: {
      bool prevent_default = false;
      NotifyWindowSystemContextMenu(GET_X_LPARAM(l_param),
                                    GET_Y_LPARAM(l_param), &prevent_default);
      return prevent_default;
    }
    case WM_SYSCOMMAND: {
      // 需要蒙版才能说明双击标题栏以最大化。
      WPARAM max_mask = 0xFFF0;
      if (transparent() && ((w_param & max_mask) == SC_MAXIMIZE)) {
        return true;
      }
      return false;
    }
    case WM_INITMENU: {
      // 这是在处理菜单可能由。
      // 用户使用“Alt+空格键”可实现系统最大化和还原。
      // 在透明窗口上使用，但这不起作用。
      if (transparent()) {
        HMENU menu = GetSystemMenu(GetAcceleratedWidget(), false);
        EnableMenuItem(menu, SC_MAXIMIZE,
                       MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        EnableMenuItem(menu, SC_RESTORE,
                       MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
        return true;
      }
      return false;
    }
    default: {
      return false;
    }
  }
}

void NativeWindowViews::HandleSizeEvent(WPARAM w_param, LPARAM l_param) {
  // 这里我们处理WM_SIZE事件，以便找出当前。
  // 窗口状态，并相应地通知用户。
  switch (w_param) {
    case SIZE_MAXIMIZED:
    case SIZE_MINIMIZED: {
      WINDOWPLACEMENT wp;
      wp.length = sizeof(WINDOWPLACEMENT);

      if (GetWindowPlacement(GetAcceleratedWidget(), &wp)) {
        last_normal_placement_bounds_ = gfx::Rect(wp.rcNormalPosition);
      }

      // 请注意，可能会发出SIZE_MAXIMIZED和SIZE_MINIMIZED。
      // 由于SetWindowPlacement调用，一次调整大小需要多次执行。
      if (w_param == SIZE_MAXIMIZED &&
          last_window_state_ != ui::SHOW_STATE_MAXIMIZED) {
        last_window_state_ = ui::SHOW_STATE_MAXIMIZED;
        NotifyWindowMaximize();
      } else if (w_param == SIZE_MINIMIZED &&
                 last_window_state_ != ui::SHOW_STATE_MINIMIZED) {
        last_window_state_ = ui::SHOW_STATE_MINIMIZED;
        NotifyWindowMinimize();
      }
      break;
    }
    case SIZE_RESTORED:
      switch (last_window_state_) {
        case ui::SHOW_STATE_MAXIMIZED:
          last_window_state_ = ui::SHOW_STATE_NORMAL;
          NotifyWindowUnmaximize();
          break;
        case ui::SHOW_STATE_MINIMIZED:
          if (IsFullscreen()) {
            last_window_state_ = ui::SHOW_STATE_FULLSCREEN;
            NotifyWindowEnterFullScreen();
          } else {
            last_window_state_ = ui::SHOW_STATE_NORMAL;
            NotifyWindowRestore();
          }
          break;
        default:
          break;
      }
      break;
  }
}

void NativeWindowViews::SetForwardMouseMessages(bool forward) {
  if (forward && !forwarding_mouse_messages_) {
    forwarding_mouse_messages_ = true;
    forwarding_windows_.insert(this);

    // 子类化用于修复转发鼠标消息时的一些问题；
    // 请参阅|SubclassProc|中的注释。
    SetWindowSubclass(legacy_window_, SubclassProc, 1,
                      reinterpret_cast<DWORD_PTR>(this));

    if (!mouse_hook_) {
      mouse_hook_ = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
    }
  } else if (!forward && forwarding_mouse_messages_) {
    forwarding_mouse_messages_ = false;
    forwarding_windows_.erase(this);

    RemoveWindowSubclass(legacy_window_, SubclassProc, 1);

    if (forwarding_windows_.empty()) {
      UnhookWindowsHookEx(mouse_hook_);
      mouse_hook_ = NULL;
    }
  }
}

LRESULT CALLBACK NativeWindowViews::SubclassProc(HWND hwnd,
                                                 UINT msg,
                                                 WPARAM w_param,
                                                 LPARAM l_param,
                                                 UINT_PTR subclass_id,
                                                 DWORD_PTR ref_data) {
  auto* window = reinterpret_cast<NativeWindowViews*>(ref_data);
  switch (msg) {
    case WM_MOUSELEAVE: {
      // 当输入被转发到底层窗口时，将发布此消息。
      // 如果不处理，它会干扰Chromium逻辑，例如。
      // 用鼠标将事件保留在要激发的位置。如果使用这些事件向前退出。
      // 模式时，会出现过度闪烁，例如将项目悬停在基础。
      // 由于快速进入和离开转发模式，可能会出现窗口。
      // 通过消费和忽略信息，我们本质上是在告诉。
      // 铬，我们没有离开窗户，尽管别人拿到了。
      // 这些信息。为什么这是为传统窗口捕获的，而不是。
      // 实际的浏览器窗口在某种程度上就是传统窗口。
      // 利用这些事件；发布到主窗口不起作用。
      if (window->forwarding_mouse_messages_) {
        return 0;
      }
      break;
    }
  }

  return DefSubclassProc(hwnd, msg, w_param, l_param);
}

LRESULT CALLBACK NativeWindowViews::MouseHookProc(int n_code,
                                                  WPARAM w_param,
                                                  LPARAM l_param) {
  if (n_code < 0) {
    return CallNextHookEx(NULL, n_code, w_param, l_param);
  }

  // 为工作区包含以下内容的窗口发布WM_MOUSEMOVE消息。
  // 游标，因为它们所处的状态否则会忽略所有。
  // 鼠标输入。
  if (w_param == WM_MOUSEMOVE) {
    for (auto* window : forwarding_windows_) {
      // 起初我考虑枚举窗口来检查光标是否。
      // 就在窗户的正上方，但由于似乎没有什么不好的事情发生。
      // 如果我们发布消息，即使其他窗口遮挡了它，我也有
      // 只是让它保持原样。
      RECT client_rect;
      GetClientRect(window->legacy_window_, &client_rect);
      POINT p = reinterpret_cast<MSLLHOOKSTRUCT*>(l_param)->pt;
      ScreenToClient(window->legacy_window_, &p);
      if (PtInRect(&client_rect, p)) {
        WPARAM w = 0;  // 没有为我们的目的按下虚拟按键。
        LPARAM l = MAKELPARAM(p.x, p.y);
        PostMessage(window->legacy_window_, WM_MOUSEMOVE, w, l);
      }
    }
  }

  return CallNextHookEx(NULL, n_code, w_param, l_param);
}

}  // 命名空间电子
