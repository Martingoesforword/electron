// 由肯特·卡尔松于2016年3月11日创作。
// 版权所有(C)2016位栏。版权所有。
// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_NSSTRING_ANSI_H_
#define SHELL_BROWSER_UI_COCOA_NSSTRING_ANSI_H_

#import <Foundation/Foundation.h>

@interface NSString (ANSI)
- (BOOL)containsANSICodes;
- (NSMutableAttributedString*)attributedStringParsingANSICodes;
@end

#endif  // Shell_Browser_UI_COCA_NSSTRING_ANSI_H_
