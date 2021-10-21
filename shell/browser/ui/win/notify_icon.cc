// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/win/notify_icon.h"

#include <objbase.h>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util_win.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "shell/browser/ui/win/notify_icon_host.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/display/screen.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace {

UINT ConvertIconType(electron::TrayIcon::IconType type) {
  using IconType = electron::TrayIcon::IconType;
  switch (type) {
    case IconType::kNone:
      return NIIF_NONE;
    case IconType::kInfo:
      return NIIF_INFO;
    case IconType::kWarning:
      return NIIF_WARNING;
    case IconType::kError:
      return NIIF_ERROR;
    case IconType::kCustom:
      return NIIF_USER;
    default:
      NOTREACHED() << "Invalid icon type";
  }
}

}  // 命名空间。

namespace electron {

NotifyIcon::NotifyIcon(NotifyIconHost* host,
                       UINT id,
                       HWND window,
                       UINT message,
                       GUID guid)
    : host_(host), icon_id_(id), window_(window), message_id_(message) {
  guid_ = guid;
  is_using_guid_ = guid != GUID_DEFAULT;
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  icon_data.uFlags |= NIF_MESSAGE;
  icon_data.uCallbackMessage = message_id_;
  BOOL result = Shell_NotifyIcon(NIM_ADD, &icon_data);
  // 当我们尝试执行以下操作时，如果资源管理器进程未运行，则可能会发生这种情况。
  // 出于某种原因(例如，在启动时)创建图标。
  if (!result)
    LOG(WARNING) << "Unable to create status tray icon.";
}

NotifyIcon::~NotifyIcon() {
  // 移除我们的图标。
  host_->Remove(this);
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  Shell_NotifyIcon(NIM_DELETE, &icon_data);
}

void NotifyIcon::HandleClickEvent(int modifiers,
                                  bool left_mouse_click,
                                  bool double_button_click) {
  gfx::Rect bounds = GetBounds();

  if (left_mouse_click) {
    if (double_button_click)  // 双击左键。
      NotifyDoubleClicked(bounds, modifiers);
    else  // 单击鼠标左键。
      NotifyClicked(bounds,
                    display::Screen::GetScreen()->GetCursorScreenPoint(),
                    modifiers);
    return;
  } else if (!double_button_click) {  // 单击鼠标右键。
    if (menu_model_)
      PopUpContextMenu(gfx::Point(), menu_model_);
    else
      NotifyRightClicked(bounds, modifiers);
  }
}

void NotifyIcon::HandleMouseMoveEvent(int modifiers) {
  gfx::Point cursorPos = display::Screen::GetScreen()->GetCursorScreenPoint();
  // 创建托盘图标但光标在托盘图标之外时触发的忽略事件。
  if (GetBounds().Contains(cursorPos))
    NotifyMouseMoved(cursorPos, modifiers);
}

void NotifyIcon::ResetIcon() {
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  // 删除任何以前存在的图标。
  Shell_NotifyIcon(NIM_DELETE, &icon_data);
  InitIconData(&icon_data);
  icon_data.uFlags |= NIF_MESSAGE;
  icon_data.uCallbackMessage = message_id_;
  icon_data.hIcon = icon_.get();
  // 如果我们有一个图像，那么设置NIF_ICON标志，它告诉我们。
  // Shell_NotifyIcon()设置其创建的状态图标的图像。
  if (icon_data.hIcon)
    icon_data.uFlags |= NIF_ICON;
  // 重新添加我们的图标。
  BOOL result = Shell_NotifyIcon(NIM_ADD, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to re-create status tray icon.";
}

void NotifyIcon::SetImage(HICON image) {
  icon_ = base::win::ScopedHICON(CopyIcon(image));

  // 创建图标。
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  icon_data.uFlags |= NIF_ICON;
  icon_data.hIcon = image;
  BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
  if (!result)
    LOG(WARNING) << "Error setting status tray icon image";
}

void NotifyIcon::SetPressedImage(HICON image) {
  // 忽略按下的图像，因为Windows上的标准是不突出显示。
  // 按下状态图标。
}

void NotifyIcon::SetToolTip(const std::string& tool_tip) {
  // 创建图标。
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  icon_data.uFlags |= NIF_TIP;
  wcsncpy_s(icon_data.szTip, base::UTF8ToWide(tool_tip).c_str(), _TRUNCATE);
  BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to set tooltip for status tray icon";
}

void NotifyIcon::DisplayBalloon(const BalloonOptions& options) {
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  icon_data.uFlags |= NIF_INFO;
  wcsncpy_s(icon_data.szInfoTitle, base::as_wcstr(options.title), _TRUNCATE);
  wcsncpy_s(icon_data.szInfo, base::as_wcstr(options.content), _TRUNCATE);
  icon_data.uTimeout = 0;
  icon_data.hBalloonIcon = options.icon;
  icon_data.dwInfoFlags = ConvertIconType(options.icon_type);

  if (options.large_icon)
    icon_data.dwInfoFlags |= NIIF_LARGE_ICON;

  if (options.no_sound)
    icon_data.dwInfoFlags |= NIIF_NOSOUND;

  if (options.respect_quiet_time)
    icon_data.dwInfoFlags |= NIIF_RESPECT_QUIET_TIME;

  BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to create status tray balloon.";
}

void NotifyIcon::RemoveBalloon() {
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  icon_data.uFlags |= NIF_INFO;

  BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to remove status tray balloon.";
}

void NotifyIcon::Focus() {
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);

  BOOL result = Shell_NotifyIcon(NIM_SETFOCUS, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to focus tray icon.";
}

void NotifyIcon::PopUpContextMenu(const gfx::Point& pos,
                                  ElectronMenuModel* menu_model) {
  // 如果未设置上下文菜单，则返回。
  if (menu_model == nullptr && menu_model_ == nullptr)
    return;

  // 将我们的窗口设置为前台窗口，以便上下文菜单在以下情况下关闭。
  // 我们点击离开它。
  if (!SetForegroundWindow(window_))
    return;

  // 取消当前菜单(如果有)。
  CloseContextMenu();

  // 默认情况下在鼠标位置显示菜单。
  gfx::Rect rect(pos, gfx::Size());
  if (pos.IsOrigin())
    rect.set_origin(display::Screen::GetScreen()->GetCursorScreenPoint());

  menu_runner_ = std::make_unique<views::MenuRunner>(
      menu_model != nullptr ? menu_model : menu_model_,
      views::MenuRunner::HAS_MNEMONICS);
  menu_runner_->RunMenuAt(nullptr, nullptr, rect,
                          views::MenuAnchorPosition::kTopLeft,
                          ui::MENU_SOURCE_MOUSE);
}

void NotifyIcon::CloseContextMenu() {
  if (menu_runner_ && menu_runner_->IsRunning()) {
    menu_runner_->Cancel();
  }
}

void NotifyIcon::SetContextMenu(ElectronMenuModel* menu_model) {
  menu_model_ = menu_model;
}

gfx::Rect NotifyIcon::GetBounds() {
  NOTIFYICONIDENTIFIER icon_id;
  memset(&icon_id, 0, sizeof(NOTIFYICONIDENTIFIER));
  icon_id.uID = icon_id_;
  icon_id.hWnd = window_;
  icon_id.cbSize = sizeof(NOTIFYICONIDENTIFIER);
  if (is_using_guid_) {
    icon_id.guidItem = guid_;
  }

  RECT rect = {0};
  Shell_NotifyIconGetRect(&icon_id, &rect);
  return display::win::ScreenWin::ScreenToDIPRect(window_, gfx::Rect(rect));
}

void NotifyIcon::InitIconData(NOTIFYICONDATA* icon_data) {
  memset(icon_data, 0, sizeof(NOTIFYICONDATA));
  icon_data->cbSize = sizeof(NOTIFYICONDATA);
  icon_data->hWnd = window_;
  icon_data->uID = icon_id_;
  if (is_using_guid_) {
    icon_data->uFlags = NIF_GUID;
    icon_data->guidItem = guid_;
  }
}

}  // 命名空间电子
