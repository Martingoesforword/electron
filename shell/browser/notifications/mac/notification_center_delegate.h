// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_CENTER_DELEGATE_H_
#define SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_CENTER_DELEGATE_H_

#import <Foundation/Foundation.h>

namespace electron {
class NotificationPresenterMac;
}

@interface NotificationCenterDelegate
    : NSObject <NSUserNotificationCenterDelegate> {
 @private
  electron::NotificationPresenterMac* presenter_;
}
- (instancetype)initWithPresenter:
    (electron::NotificationPresenterMac*)presenter;
@end

#endif  // SHELL_BROWSER_NOTIFICATIONS_MAC_NOTIFICATION_CENTER_DELEGATE_H_
