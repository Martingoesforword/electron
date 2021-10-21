// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#import <Cocoa/Cocoa.h>

#import "shell/browser/ui/cocoa/electron_menu_controller.h"

@interface ElectronApplicationDelegate : NSObject <NSApplicationDelegate> {
 @private
  base::scoped_nsobject<ElectronMenuController> menu_controller_;
}

// 设置将在“application ationDockMenu：”中返回的菜单。
- (void)setApplicationDockMenu:(electron::ElectronMenuModel*)model;

@end
