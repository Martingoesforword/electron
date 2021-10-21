// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_DELEGATE_H_
#define SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_DELEGATE_H_

#include <string>

namespace electron {

class NotificationDelegate {
 public:
  // 本机通知对象被销毁。
  virtual void NotificationDestroyed() {}

  // 无法发送通知。
  virtual void NotificationFailed(const std::string& error) {}

  // 通知已回复。
  virtual void NotificationReplied(const std::string& reply) {}
  virtual void NotificationAction(int index) {}

  virtual void NotificationClick() {}
  virtual void NotificationClosed() {}
  virtual void NotificationDisplayed() {}

 protected:
  NotificationDelegate() = default;
  ~NotificationDelegate() = default;
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_DELEGATE_H_
