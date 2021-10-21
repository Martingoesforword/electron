// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_
#define SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_

#import <Foundation/Foundation.h>

#include <map>
#include <string>

#include "base/mac/scoped_nsobject.h"
#include "shell/browser/notifications/notification.h"

namespace electron {

class CocoaNotification : public Notification {
 public:
  CocoaNotification(NotificationDelegate* delegate,
                    NotificationPresenter* presenter);
  ~CocoaNotification() override;

  // 通知：
  void Show(const NotificationOptions& options) override;
  void Dismiss() override;

  void NotificationDisplayed();
  void NotificationReplied(const std::string& reply);
  void NotificationActivated();
  void NotificationActivated(NSUserNotificationAction* action);
  void NotificationDismissed();

  NSUserNotification* notification() const { return notification_; }

 private:
  void LogAction(const char* action);

  base::scoped_nsobject<NSUserNotification> notification_;
  std::map<std::string, unsigned> additional_action_indices_;
  unsigned action_index_;

  DISALLOW_COPY_AND_ASSIGN(CocoaNotification);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NOTIFICATIONS_MAC_COCOA_NOTIFICATION_H_
