// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/notifications/win/notification_presenter_win7.h"

#include <string>

#include "shell/browser/notifications/win/win32_notification.h"

namespace electron {

electron::Notification* NotificationPresenterWin7::CreateNotificationObject(
    NotificationDelegate* delegate) {
  return new Win32Notification(delegate, this);
}

Win32Notification* NotificationPresenterWin7::GetNotificationObjectByRef(
    const DesktopNotificationController::Notification& ref) {
  for (auto* n : this->notifications()) {
    auto* w32n = static_cast<Win32Notification*>(n);
    if (w32n->GetRef() == ref)
      return w32n;
  }

  return nullptr;
}

Win32Notification* NotificationPresenterWin7::GetNotificationObjectByTag(
    const std::string& tag) {
  for (auto* n : this->notifications()) {
    auto* w32n = static_cast<Win32Notification*>(n);
    if (w32n->GetTag() == tag)
      return w32n;
  }

  return nullptr;
}

void NotificationPresenterWin7::OnNotificationClicked(
    const Notification& notification) {
  auto* n = GetNotificationObjectByRef(notification);
  if (n)
    n->NotificationClicked();
}

void NotificationPresenterWin7::OnNotificationDismissed(
    const Notification& notification) {
  auto* n = GetNotificationObjectByRef(notification);
  if (n)
    n->NotificationDismissed();
}

}  // 命名空间电子
