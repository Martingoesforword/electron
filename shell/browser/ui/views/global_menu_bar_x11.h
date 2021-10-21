// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_X11_H_
#define SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_X11_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/x/xproto.h"

typedef struct _DbusmenuMenuitem DbusmenuMenuitem;
typedef struct _DbusmenuServer DbusmenuServer;

namespace ui {
class Accelerator;
}

namespace electron {

class NativeWindowViews;

// 控制Unity上的Mac样式菜单栏。
// 
// Unity在屏幕顶部有一个类似Apple的菜单栏，可以更改。
// 取决于活动窗口。在GTK端口中，我们有一个隐藏的GtkMenuBar。
// 对象在每个GtkWindow中，该对象的存在只是为了由。
// LibdbusMenu-GTK代码。因为我们不再有GtkWindows，所以我们需要。
// 直接与较低级别的libdbusmenu-lib接口，我们。
// 机会主义的dlopen()，因为并不是每个人都在运行Ubuntu。
// 
// 此类类似于Chrome的相应类，但它生成菜单。
// 取而代之的是菜单模型，而且它也是特定于每个窗口的。
class GlobalMenuBarX11 {
 public:
  explicit GlobalMenuBarX11(NativeWindowViews* window);
  virtual ~GlobalMenuBarX11();

  // 创建附加到|Window|的DbusmenuServer的对象路径。
  static std::string GetPathForWindow(x11::Window window);

  void SetMenu(ElectronMenuModel* menu_model);
  bool IsServerStarted() const;

  // 显示/隐藏时由NativeWindow调用。
  void OnWindowMapped();
  void OnWindowUnmapped();

 private:
  // 创建一个DbusmenuServer。
  void InitServer(x11::Window window);

  // 从菜单模型创建菜单。
  void BuildMenuFromModel(ElectronMenuModel* model, DbusmenuMenuitem* parent);

  // 设置|Item|的快捷键。
  void RegisterAccelerator(DbusmenuMenuitem* item,
                           const ui::Accelerator& accelerator);

  CHROMEG_CALLBACK_1(GlobalMenuBarX11,
                     void,
                     OnItemActivated,
                     DbusmenuMenuitem*,
                     unsigned int);
  CHROMEG_CALLBACK_0(GlobalMenuBarX11, void, OnSubMenuShow, DbusmenuMenuitem*);

  NativeWindowViews* window_;
  x11::Window xwindow_;

  DbusmenuServer* server_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(GlobalMenuBarX11);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Views_GLOBAL_MENU_BAR_X11_H_
