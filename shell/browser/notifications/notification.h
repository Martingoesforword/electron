// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_H_
#define SHELL_BROWSER_NOTIFICATIONS_NOTIFICATION_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

namespace electron {

class NotificationDelegate;
class NotificationPresenter;

struct NotificationAction {
  std::u16string type;
  std::u16string text;
};

struct NotificationOptions {
  std::u16string title;
  std::u16string subtitle;
  std::u16string msg;
  std::string tag;
  bool silent;
  GURL icon_url;
  SkBitmap icon;
  bool has_reply;
  std::u16string timeout_type;
  std::u16string reply_placeholder;
  std::u16string sound;
  std::u16string urgency;  // Linux。
  std::vector<NotificationAction> actions;
  std::u16string close_button_text;
  std::u16string toast_xml;

  NotificationOptions();
  ~NotificationOptions();
};

class Notification {
 public:
  virtual ~Notification();

  // 显示通知。
  virtual void Show(const NotificationOptions& options) = 0;
  // 关闭通知，则此实例将在。
  // 通知将关闭。
  virtual void Dismiss() = 0;

  // 应由派生类调用。
  void NotificationClicked();
  void NotificationDismissed();
  void NotificationFailed(const std::string& error = "");

  // 把这个删掉。
  void Destroy();

  base::WeakPtr<Notification> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void set_delegate(NotificationDelegate* delegate) { delegate_ = delegate; }
  void set_notification_id(const std::string& id) { notification_id_ = id; }

  NotificationDelegate* delegate() const { return delegate_; }
  NotificationPresenter* presenter() const { return presenter_; }
  const std::string& notification_id() const { return notification_id_; }

 protected:
  Notification(NotificationDelegate* delegate,
               NotificationPresenter* presenter);

 private:
  NotificationDelegate* delegate_;
  NotificationPresenter* presenter_;
  std::string notification_id_;

  base::WeakPtrFactory<Notification> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(Notification);
};

}  // 命名空间电子。

#endif  // 外壳浏览器通知通知H_
