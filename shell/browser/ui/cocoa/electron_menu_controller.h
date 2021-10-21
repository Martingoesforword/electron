// 版权所有(C)2013 GitHub，Inc.。
// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_ELECTRON_MENU_CONTROLLER_H_
#define SHELL_BROWSER_UI_COCOA_ELECTRON_MENU_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/callback.h"
#include "base/mac/scoped_nsobject.h"
#include "base/memory/weak_ptr.h"

namespace electron {
class ElectronMenuModel;
}

// 用于跨平台菜单模型的控制器。创建的菜单。
// 为每个菜单项设置标签和表示的对象。该对象是一个。
// NSValue保持指向该级别菜单的模型的指针(到。
// 允许分层菜单)。标记是进入该模型的索引。
// 那件特别的物品。很重要的一点是，模型的寿命要比该对象的寿命长。
// 因为它只维护弱引用。
@interface ElectronMenuController
    : NSObject <NSMenuDelegate, NSSharingServiceDelegate> {
 @protected
  base::WeakPtr<electron::ElectronMenuModel> model_;
  base::scoped_nsobject<NSMenu> menu_;
  BOOL isMenuOpen_;
  BOOL useDefaultAccelerator_;
  base::OnceClosure closeCallback;
}

// 从预置模型构建NSMenu(不得为空)。所做的更改。
// 调用此函数后将不会注意到模型的内容。
- (id)initWithModel:(electron::ElectronMenuModel*)model
    useDefaultAccelerator:(BOOL)use;

- (void)setCloseCallback:(base::OnceClosure)callback;

// 使用|型号|填充当前NSMenu。
- (void)populateWithModel:(electron::ElectronMenuModel*)model;

// 以编程方式关闭构造的菜单。
- (void)cancel;

- (electron::ElectronMenuModel*)model;
- (void)setModel:(electron::ElectronMenuModel*)model;

// 如果使用了复杂初始值设定项，则访问构造的菜单。如果。
// 使用了默认初始值设定项，然后这将在第一次调用时创建菜单。
- (NSMenu*)menu;

- (base::scoped_nsobject<NSMenuItem>)
    makeMenuItemForIndex:(NSInteger)index
               fromModel:(electron::ElectronMenuModel*)model;

// 菜单当前是否打开。
- (BOOL)isMenuOpen;

// 此类实现的NSMenuDelegate方法。子类应调用Super类。
// 如果扩展行为的话。
- (void)menuWillOpen:(NSMenu*)menu;
- (void)menuDidClose:(NSMenu*)menu;

@end

#endif  // SHELL_BROWSER_UI_COCOA_ELECTRON_MENU_CONTROLLER_H_
