// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NOTIFICATIONS_LINUX_LIBNOTIFY_NOTIFICATION_H_
#define SHELL_BROWSER_NOTIFICATIONS_LINUX_LIBNOTIFY_NOTIFICATION_H_

#include "library_loaders/libnotify_loader.h"
#include "shell/browser/notifications/notification.h"
#include "ui/base/glib/glib_signal.h"

namespace electron {

class LibnotifyNotification : public Notification {
 public:
  LibnotifyNotification(NotificationDelegate* delegate,
                        NotificationPresenter* presenter);
  ~LibnotifyNotification() override;

  static bool Initialize();

  // 通知：
  void Show(const NotificationOptions& options) override;
  void Dismiss() override;

 private:
  CHROMEG_CALLBACK_0(LibnotifyNotification,
                     void,
                     OnNotificationClosed,
                     NotifyNotification*);
  CHROMEG_CALLBACK_1(LibnotifyNotification,
                     void,
                     OnNotificationView,
                     NotifyNotification*,
                     char*);

  NotifyNotification* notification_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(LibnotifyNotification);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NOTIFICATIONS_LINUX_LIBNOTIFY_NOTIFICATION_H_
