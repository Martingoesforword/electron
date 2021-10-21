// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/notifications/notification.h"

#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"

namespace electron {

NotificationOptions::NotificationOptions() = default;
NotificationOptions::~NotificationOptions() = default;

Notification::Notification(NotificationDelegate* delegate,
                           NotificationPresenter* presenter)
    : delegate_(delegate), presenter_(presenter) {}

Notification::~Notification() {
  if (delegate())
    delegate()->NotificationDestroyed();
}

void Notification::NotificationClicked() {
  if (delegate())
    delegate()->NotificationClick();
  Destroy();
}

void Notification::NotificationDismissed() {
  if (delegate())
    delegate()->NotificationClosed();
  Destroy();
}

void Notification::NotificationFailed(const std::string& error) {
  if (delegate())
    delegate()->NotificationFailed(error);
  Destroy();
}

void Notification::Destroy() {
  presenter()->RemoveNotification(this);
}

}  // 命名空间电子
