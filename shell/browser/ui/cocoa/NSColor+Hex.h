// 由Mathias Leppich于14年2月3日创作。
// 版权所有(C)2014位栏。版权所有。
// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_NSCOLOR_HEX_H_
#define SHELL_BROWSER_UI_COCOA_NSCOLOR_HEX_H_

#import <Cocoa/Cocoa.h>

@interface NSColor (Hex)
- (NSString*)hexadecimalValue;
- (NSString*)RGBAValue;
+ (NSColor*)colorWithHexColorString:(NSString*)hex;
@end

#endif  // Shell_Browser_UI_COCA_NSCOLOR_HEX_H_
