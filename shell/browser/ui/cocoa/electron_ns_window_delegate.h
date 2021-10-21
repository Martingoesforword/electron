// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_DELEGATE_H_
#define SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_DELEGATE_H_

#include <Quartz/Quartz.h>

#include "components/remote_cocoa/app_shim/views_nswindow_delegate.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace electron {
class NativeWindowMac;
}

@interface ElectronNSWindowDelegate
    : ViewsNSWindowDelegate <NSTouchBarDelegate, QLPreviewPanelDataSource> {
 @private
  electron::NativeWindowMac* shell_;
  bool is_zooming_;
  int level_;
  bool is_resizable_;

  // 仅在实时调整大小期间有效。
  // 用于跟踪调整大小是水平进行还是。
  // 垂直方向，即使在物理上用户在两个方向上都在调整大小。
  absl::optional<bool> resizingHorizontally_;
}
- (id)initWithShell:(electron::NativeWindowMac*)shell;
@end

#endif  // SHELL_BROWSER_UI_COCOA_ELECTRON_NS_WINDOW_DELEGATE_H_
