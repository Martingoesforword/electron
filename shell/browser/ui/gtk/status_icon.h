// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

// 版权所有(C)2020 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_GTK_STATUS_ICON_H_
#define SHELL_BROWSER_UI_GTK_STATUS_ICON_H_

#include <memory>

#include "base/strings/string_util.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/linux_ui/status_icon_linux.h"

namespace electron {

namespace gtkui {

bool IsStatusIconSupported();
std::unique_ptr<views::StatusIconLinux> CreateLinuxStatusIcon(
    const gfx::ImageSkia& image,
    const std::u16string& tool_tip,
    const char* id_prefix);

}  // 命名空间gtkui。

}  // 命名空间电子。

#endif  // Shell_Browser_UI_GTK_STATUS_ICON_H_
