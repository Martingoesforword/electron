// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN7_H_
#define SHELL_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN7_H_

#include <string>

#include "shell/browser/notifications/notification_presenter.h"
#include "shell/browser/notifications/win/win32_desktop_notifications/desktop_notification_controller.h"

namespace electron {

class Win32Notification;

class NotificationPresenterWin7 : public NotificationPresenter,
                                  public DesktopNotificationController {
 public:
  NotificationPresenterWin7() = default;

  Win32Notification* GetNotificationObjectByRef(
      const DesktopNotificationController::Notification& ref);

  Win32Notification* GetNotificationObjectByTag(const std::string& tag);

 private:
  electron::Notification* CreateNotificationObject(
      NotificationDelegate* delegate) override;

  void OnNotificationClicked(const Notification& notification) override;
  void OnNotificationDismissed(const Notification& notification) override;

  DISALLOW_COPY_AND_ASSIGN(NotificationPresenterWin7);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN7_H_
