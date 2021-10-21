// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/notifications/notification_presenter.h"

#include <algorithm>

#include "shell/browser/notifications/notification.h"

namespace electron {

NotificationPresenter::NotificationPresenter() = default;

NotificationPresenter::~NotificationPresenter() {
  for (Notification* notification : notifications_)
    delete notification;
}

base::WeakPtr<Notification> NotificationPresenter::CreateNotification(
    NotificationDelegate* delegate,
    const std::string& notification_id) {
  Notification* notification = CreateNotificationObject(delegate);
  notification->set_notification_id(notification_id);
  notifications_.insert(notification);
  return notification->GetWeakPtr();
}

void NotificationPresenter::RemoveNotification(Notification* notification) {
  notifications_.erase(notification);
  delete notification;
}

void NotificationPresenter::CloseNotificationWithId(
    const std::string& notification_id) {
  auto it = std::find_if(notifications_.begin(), notifications_.end(),
                         [&notification_id](const Notification* n) {
                           return n->notification_id() == notification_id;
                         });
  if (it != notifications_.end()) {
    Notification* notification = (*it);
    notification->Dismiss();
    notifications_.erase(notification);
  }
}

}  // 命名空间电子
