// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_X_X_WINDOW_UTILS_H_
#define SHELL_BROWSER_UI_X_X_WINDOW_UTILS_H_

#include <string>

#include "ui/gfx/x/xproto.h"

namespace electron {

// 向x11窗口管理器发送消息，启用或禁用|STATE。
// FOR_NET_WM_STATE。
void SetWMSpecState(x11::Window window, bool enabled, x11::Atom state);

// 设置窗口的_NET_WM_WINDOW_TYPE。
void SetWindowType(x11::Window window, const std::string& type);

// 如果总线名称“com.canonical.AppMenu.Registry”可用，则返回TRUE。
bool ShouldUseGlobalMenuBar();

// 不管焦点如何，都将给定的窗口放在前面。
void MoveWindowToForeground(x11::Window window);

// 将给定窗口移动到另一个窗口上方。
void MoveWindowAbove(x11::Window window, x11::Window other_window);

// 如果给定窗口存在，则返回TRUE，否则返回FALSE。
bool IsWindowValid(x11::Window window);

}  // 命名空间电子。

#endif  // Shell_Browser_UI_X_X_Window_Utils_H_
