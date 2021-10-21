// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_EVENT_DISPATCHING_WINDOW_H_
#define SHELL_BROWSER_UI_COCOA_EVENT_DISPATCHING_WINDOW_H_

#import "ui/base/cocoa/underlay_opengl_hosting_window.h"

@interface EventDispatchingWindow : UnderlayOpenGLHostingWindow {
 @private
  BOOL redispatchingEvent_;
}

- (void)redispatchKeyEvent:(NSEvent*)event;

@end

#endif  // SHELL_BROWSER_UI_COCOA_EVENT_DISPATCHING_WINDOW_H_
