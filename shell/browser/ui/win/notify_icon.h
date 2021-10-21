// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_WIN_NOTIFY_ICON_H_
#define SHELL_BROWSER_UI_WIN_NOTIFY_ICON_H_

#include <windows.h>  // 必须先包含windows.h。

#include <shellapi.h>

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/win/scoped_gdi_object.h"
#include "shell/browser/ui/tray_icon.h"
#include "shell/browser/ui/win/notify_icon_host.h"

namespace gfx {
class Point;
}

namespace views {
class MenuRunner;
}

namespace electron {

class NotifyIconHost;

class NotifyIcon : public TrayIcon {
 public:
  // 构造函数，该构造函数提供该图标的唯一ID和消息窗口。
  NotifyIcon(NotifyIconHost* host,
             UINT id,
             HWND window,
             UINT message,
             GUID guid);
  ~NotifyIcon() override;

  // 处理来自用户的单击事件-如果|LEFT_BUTTON_CLICK|为TRUE且。
  // 存在注册的观察者，将点击事件传递给观察者，
  // 否则将显示上下文菜单(如果有)。
  void HandleClickEvent(int modifiers,
                        bool left_button_click,
                        bool double_button_click);

  // 处理来自用户的鼠标移动事件。
  void HandleMouseMoveEvent(int modifiers);

  // 任务栏创建后，立即重新创建状态托盘图标。
  void ResetIcon();

  UINT icon_id() const { return icon_id_; }
  HWND window() const { return window_; }
  UINT message_id() const { return message_id_; }
  GUID guid() const { return guid_; }

  // 从TrayIcon覆盖：
  void SetImage(HICON image) override;
  void SetPressedImage(HICON image) override;
  void SetToolTip(const std::string& tool_tip) override;
  void DisplayBalloon(const BalloonOptions& options) override;
  void RemoveBalloon() override;
  void Focus() override;
  void PopUpContextMenu(const gfx::Point& pos,
                        ElectronMenuModel* menu_model) override;
  void CloseContextMenu() override;
  void SetContextMenu(ElectronMenuModel* menu_model) override;
  gfx::Rect GetBounds() override;

  base::WeakPtr<NotifyIcon> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

 private:
  void InitIconData(NOTIFYICONDATA* icon_data);

  // 拥有我们的托盘。瘦弱。
  NotifyIconHost* host_;

  // 与此图标对应的唯一ID。
  UINT icon_id_;

  // 用于处理来自此图标的消息的窗口。
  HWND window_;

  // 用于状态图标消息的消息标识符。
  UINT message_id_;

  // 窗口的当前显示图标。
  base::win::ScopedHICON icon_;

  // 上下文菜单。
  ElectronMenuModel* menu_model_ = nullptr;

  // 用于在Windows上标识任务栏条目的可选GUID。
  GUID guid_ = GUID_DEFAULT;

  // 指示托盘条目是否与GUID关联。
  bool is_using_guid_ = false;

  // 与此图标关联的上下文菜单(如果有)。
  std::unique_ptr<views::MenuRunner> menu_runner_;

  base::WeakPtrFactory<NotifyIcon> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(NotifyIcon);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Win_Notify_ICON_H_
