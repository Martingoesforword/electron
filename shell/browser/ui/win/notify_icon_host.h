// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_WIN_NOTIFY_ICON_HOST_H_
#define SHELL_BROWSER_UI_WIN_NOTIFY_ICON_HOST_H_

#include <windows.h>

#include <vector>

#include "base/macros.h"
#include "shell/common/gin_converters/guid_converter.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

const GUID GUID_DEFAULT = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

namespace electron {

class NotifyIcon;

class NotifyIconHost {
 public:
  NotifyIconHost();
  ~NotifyIconHost();

  NotifyIcon* CreateNotifyIcon(absl::optional<UUID> guid);
  void Remove(NotifyIcon* notify_icon);

 private:
  typedef std::vector<NotifyIcon*> NotifyIcons;

  // 当消息进入我们的消息传递窗口时调用静态回调。
  static LRESULT CALLBACK WndProcStatic(HWND hwnd,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam);

  LRESULT CALLBACK WndProc(HWND hwnd,
                           UINT message,
                           WPARAM wparam,
                           LPARAM lparam);

  UINT NextIconId();

  // 我们将分配给下一个图标的唯一图标ID。
  UINT next_icon_id_ = 1;

  // 包含所有活动NotifyIcons的列表。
  NotifyIcons notify_icons_;

  // |Window_|的窗口类。
  ATOM atom_ = 0;

  // 包含|Window_|的窗口过程的模块的句柄。
  HMODULE instance_ = nullptr;

  // 用于处理事件的窗口。
  HWND window_ = nullptr;

  // “TaskbarCreated”消息的消息ID，在我们需要时发送给我们。
  // 重置我们的状态图标。
  UINT taskbar_created_message_ = 0;

  DISALLOW_COPY_AND_ASSIGN(NotifyIconHost);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Win_Notify_ICON_HOST_H_
