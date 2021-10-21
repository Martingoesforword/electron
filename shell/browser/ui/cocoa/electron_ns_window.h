// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_H_
#define SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_H_

#include "components/remote_cocoa/app_shim/native_widget_mac_nswindow.h"
#include "shell/browser/ui/cocoa/event_dispatching_window.h"
#include "ui/views/widget/native_widget_mac.h"

namespace electron {

class NativeWindowMac;

// 防止窗口在作用域期间调整大小。
class ScopedDisableResize {
 public:
  ScopedDisableResize() { disable_resize_ = true; }
  ~ScopedDisableResize() { disable_resize_ = false; }

  static bool IsResizeDisabled() { return disable_resize_; }

 private:
  static bool disable_resize_;
};

}  // 命名空间电子。

@interface ElectronNSWindow : NativeWidgetMacNSWindow {
 @private
  electron::NativeWindowMac* shell_;
}
@property BOOL acceptsFirstMouse;
@property BOOL enableLargerThanScreen;
@property BOOL disableAutoHideCursor;
@property BOOL disableKeyOrMainWindow;
@property(nonatomic, retain) NSVisualEffectView* vibrantView;
@property(nonatomic, retain) NSImage* cornerMask;
- (id)initWithShell:(electron::NativeWindowMac*)shell
          styleMask:(NSUInteger)styleMask;
- (electron::NativeWindowMac*)shell;
- (id)accessibilityFocusedUIElement;
- (NSRect)originalContentRectForFrameRect:(NSRect)frameRect;
- (void)toggleFullScreenMode:(id)sender;
- (NSImage*)_cornerMask;
@end

#endif  // Shell_Browser_UI_Cocoa_Electronics_NS_Window_H_
