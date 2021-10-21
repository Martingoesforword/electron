// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_NOTIFICATION_H_
#define SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_NOTIFICATION_H_

#include <string>

#include "shell/browser/notifications/notification.h"
#include "shell/browser/notifications/win/notification_presenter_win7.h"

namespace electron {

class Win32Notification : public electron::Notification {
 public:
  Win32Notification(NotificationDelegate* delegate,
                    NotificationPresenterWin7* presenter)
      : Notification(delegate, presenter) {}
  void Show(const NotificationOptions& options) override;
  void Dismiss() override;

  const DesktopNotificationController::Notification& GetRef() const {
    return notification_ref_;
  }

  const std::string& GetTag() const { return tag_; }

 private:
  DesktopNotificationController::Notification notification_ref_;
  std::string tag_;

  DISALLOW_COPY_AND_ASSIGN(Win32Notification);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_NOTIFICATION_H_
