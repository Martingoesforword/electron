// 版权所有(C)2020 Microsoft Inc.保留所有权利。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_WIN_DARK_MODE_H_
#define SHELL_BROWSER_WIN_DARK_MODE_H_

#ifdef WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

#include "ui/native_theme/native_theme.h"

namespace electron {

namespace win {

void SetDarkModeForWindow(HWND hWnd, ui::NativeTheme::ThemeSource theme_source);

}  // 命名空间制胜。

}  // 命名空间电子。

#endif  // Shell_Browser_Win_Dark_MODE_H_
