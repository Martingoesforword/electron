// 版权所有(C)2021 Microsoft，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_PROXY_H_
#define SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_PROXY_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/gfx/geometry/point.h"

@class WindowButtonsProxy;

// 浮动在窗口按钮上方的帮助器视图。
@interface ButtonsAreaHoverView : NSView {
 @private
  WindowButtonsProxy* proxy_;
}
- (id)initWithProxy:(WindowButtonsProxy*)proxy;
@end

// 操纵窗口按钮。
@interface WindowButtonsProxy : NSObject {
 @private
  NSWindow* window_;

  // 按钮的当前左上角空白。
  gfx::Point margin_;
  // 默认的左上角页边距。
  gfx::Point default_margin_;

  // 跟踪鼠标在窗口按钮上方的移动。
  BOOL show_on_hover_;
  BOOL mouse_inside_;
  base::scoped_nsobject<NSTrackingArea> tracking_area_;
  base::scoped_nsobject<ButtonsAreaHoverView> hover_view_;
}

- (id)initWithWindow:(NSWindow*)window;

- (void)setVisible:(BOOL)visible;
- (BOOL)isVisible;

// 仅当鼠标移动到窗口按钮上方时才显示窗口按钮。
- (void)setShowOnHover:(BOOL)yes;

// 设置窗口按钮的左上角空白。
- (void)setMargin:(const absl::optional<gfx::Point>&)margin;

// 返回所有3个按钮的边界，所有边距均为边距。
- (NSRect)getButtonsContainerBounds;

- (void)redraw;
- (void)updateTrackingAreas;
@end

#endif  // SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_PROXY_H_
