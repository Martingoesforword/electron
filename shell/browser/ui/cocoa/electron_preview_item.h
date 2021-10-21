// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_ELECTRON_PREVIEW_ITEM_H_
#define SHELL_BROWSER_UI_COCOA_ELECTRON_PREVIEW_ITEM_H_

#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>

@interface ElectronPreviewItem : NSObject <QLPreviewItem>
@property(nonatomic, retain) NSURL* previewItemURL;
@property(nonatomic, retain) NSString* previewItemTitle;
- (id)initWithURL:(NSURL*)url title:(NSString*)title;
@end

#endif  // SHELL_BROWSER_UI_COCOA_ELECTRON_PREVIEW_ITEM_H_
