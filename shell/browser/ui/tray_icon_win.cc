// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/win/notify_icon.h"
#include "shell/browser/ui/win/notify_icon_host.h"

namespace electron {

// 静电。
TrayIcon* TrayIcon::Create(absl::optional<UUID> guid) {
  static NotifyIconHost host;
  return host.CreateNotifyIcon(guid);
}

}  // 命名空间电子
