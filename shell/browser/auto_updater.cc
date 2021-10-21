// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/auto_updater.h"

namespace auto_updater {

Delegate* AutoUpdater::delegate_ = nullptr;

Delegate* AutoUpdater::GetDelegate() {
  return delegate_;
}

void AutoUpdater::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

#if !defined(OS_MAC) || defined(MAS_BUILD)
std::string AutoUpdater::GetFeedURL() {
  return "";
}

void AutoUpdater::SetFeedURL(gin::Arguments* args) {}

void AutoUpdater::CheckForUpdates() {}

void AutoUpdater::QuitAndInstall() {}
#endif

}  // 命名空间自动更新程序(_U)
