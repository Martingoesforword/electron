// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "base/callback.h"
#include "base/mac/scoped_nsobject.h"
#include "base/mac/scoped_sending_event.h"

#import <AVFoundation/AVFoundation.h>
#import <LocalAuthentication/LocalAuthentication.h>

@interface AtomApplication : NSApplication <CrAppProtocol,
                                            CrAppControlProtocol,
                                            NSUserActivityDelegate> {
 @private
  BOOL handlingSendEvent_;
  base::scoped_nsobject<NSUserActivity> currentActivity_;
  NSCondition* handoffLock_;
  BOOL updateReceived_;
  BOOL userStoppedShutdown_;
  base::RepeatingCallback<bool()> shouldShutdown_;
}

+ (AtomApplication*)sharedApplication;

- (void)setShutdownHandler:(base::RepeatingCallback<bool()>)handler;
- (void)registerURLHandler;

// 在MacOS本身关闭时调用。
- (void)willPowerOff:(NSNotification*)notify;

// CrAppProtocol：
- (BOOL)isHandlingSendEvent;

// CrAppControlProtocol：
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;

- (NSUserActivity*)getCurrentActivity;
- (void)setCurrentActivity:(NSString*)type
              withUserInfo:(NSDictionary*)userInfo
            withWebpageURL:(NSURL*)webpageURL;
- (void)invalidateCurrentActivity;
- (void)resignCurrentActivity;
- (void)updateCurrentActivity:(NSString*)type
                 withUserInfo:(NSDictionary*)userInfo;

@end
