// 版权所有(C)2012 Chromium作者。版权所有。
// 版权所有(C)2013 Patrick Reynolds&lt;piki@github.com&gt;。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_NOTIFICATIONS_LINUX_NOTIFICATION_PRESENTER_LINUX_H_
#define SHELL_BROWSER_NOTIFICATIONS_LINUX_NOTIFICATION_PRESENTER_LINUX_H_

#include "shell/browser/notifications/notification_presenter.h"

namespace electron {

class NotificationPresenterLinux : public NotificationPresenter {
 public:
  NotificationPresenterLinux();
  ~NotificationPresenterLinux() override;

 private:
  Notification* CreateNotificationObject(
      NotificationDelegate* delegate) override;

  DISALLOW_COPY_AND_ASSIGN(NotificationPresenterLinux);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NOTIFICATIONS_LINUX_NOTIFICATION_PRESENTER_LINUX_H_
