// 版权所有(C)2012 Chromium作者。版权所有。
// 版权所有(C)2013 Adam Roben&lt;adam@roben.org&gt;。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_PRESENTER_MAC_H_
#define SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_PRESENTER_MAC_H_

#include "base/mac/scoped_nsobject.h"
#include "shell/browser/notifications/mac/notification_center_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"

namespace electron {

class CocoaNotification;

class NotificationPresenterMac : public NotificationPresenter {
 public:
  CocoaNotification* GetNotification(NSUserNotification* ns_notification);

  NotificationPresenterMac();
  ~NotificationPresenterMac() override;

 private:
  Notification* CreateNotificationObject(
      NotificationDelegate* delegate) override;

  base::scoped_nsobject<NotificationCenterDelegate>
      notification_center_delegate_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPresenterMac);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_PRESENTER_MAC_H_
