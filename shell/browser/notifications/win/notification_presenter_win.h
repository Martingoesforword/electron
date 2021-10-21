// 版权所有(C)2012 Chromium作者。版权所有。
// 版权所有(C)2015 Felix Rieseberg&lt;feriese@microsoft.com&gt;和。
// Jason Poon&lt;jason.poon@microsoft.com&gt;。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

// 用法示例(JavaScript：
// Var windowsNotification=新通知(“测试标题”，{。
// 身体：“嗨，我是个榜样。你好吗？”
// 图标：“file:///C:/Path/To/Your/Image.png”
// })；

// WindowsNotification.onshow=function(){。
// Console.log(“显示的通知”)。
// }；
// WindowsNotification.onclick=函数(){。
// Console.log(“通知已点击”)。
// }；
// WindowsNotification.onclose=函数(){。
// Console.log(“通知撤销”)。
// }；

#ifndef SHELL_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN_H_
#define SHELL_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN_H_

#include "base/files/scoped_temp_dir.h"
#include "shell/browser/notifications/notification_presenter.h"

class GURL;
class SkBitmap;

namespace electron {

class NotificationPresenterWin : public NotificationPresenter {
 public:
  NotificationPresenterWin();
  ~NotificationPresenterWin() override;

  bool Init();

  std::wstring SaveIconToFilesystem(const SkBitmap& icon, const GURL& origin);

 private:
  Notification* CreateNotificationObject(
      NotificationDelegate* delegate) override;

  base::ScopedTempDir temp_dir_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPresenterWin);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NOTIFICATIONS_WIN_NOTIFICATION_PRESENTER_WIN_H_
