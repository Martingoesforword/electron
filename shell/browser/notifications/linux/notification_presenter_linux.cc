// 版权所有(C)2012 Chromium作者。版权所有。
// 版权所有(C)2013 Patrick Reynolds&lt;piki@github.com&gt;。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#include "shell/browser/notifications/linux/notification_presenter_linux.h"

#include "shell/browser/notifications/linux/libnotify_notification.h"

namespace electron {

// 静电。
NotificationPresenter* NotificationPresenter::Create() {
  if (!LibnotifyNotification::Initialize())
    return nullptr;
  return new NotificationPresenterLinux;
}

NotificationPresenterLinux::NotificationPresenterLinux() = default;

NotificationPresenterLinux::~NotificationPresenterLinux() = default;

Notification* NotificationPresenterLinux::CreateNotificationObject(
    NotificationDelegate* delegate) {
  return new LibnotifyNotification(delegate, this);
}

}  // 命名空间电子
