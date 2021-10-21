// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

// 版权所有(C)2020 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/gtk/status_icon.h"

#include <gtk/gtk.h>

#include <memory>

#include "base/strings/stringprintf.h"
#include "shell/browser/ui/gtk/app_indicator_icon.h"
#include "shell/browser/ui/gtk/gtk_status_icon.h"

namespace electron {

namespace gtkui {

namespace {

int indicators_count = 0;

}

bool IsStatusIconSupported() {
#if GTK_CHECK_VERSION(3, 90, 0)
  NOTIMPLEMENTED();
  return false;
#else
  return true;
#endif
}

std::unique_ptr<views::StatusIconLinux> CreateLinuxStatusIcon(
    const gfx::ImageSkia& image,
    const std::u16string& tool_tip,
    const char* id_prefix) {
#if GTK_CHECK_VERSION(3, 90, 0)
  NOTIMPLEMENTED();
  return nullptr;
#else
  if (AppIndicatorIcon::CouldOpen()) {
    ++indicators_count;

    return std::make_unique<AppIndicatorIcon>(
        base::StringPrintf("%s%d", id_prefix, indicators_count), image,
        tool_tip);
  } else {
    return std::make_unique<GtkStatusIcon>(image, tool_tip);
  }
#endif
}

}  // 命名空间gtkui。

}  // 命名空间电子
