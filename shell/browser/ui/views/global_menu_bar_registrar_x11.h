// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_
#define SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_

#include <gio/gio.h>

#include <set>

#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/gfx/x/xproto.h"

// 向Unity宣传我们的菜单栏。
// 
// GlobalMenuBarX11负责管理每个服务器的DbusmenuServer。
// X11：：Window。我们需要一个单独的对象来拥有dbus通道。
// Com.canonical.AppMenu.Registry并注册/取消注册映射。
// 在x11：：Window和我们提供的DbusmenuServer实例之间。
class GlobalMenuBarRegistrarX11 {
 public:
  static GlobalMenuBarRegistrarX11* GetInstance();

  void OnWindowMapped(x11::Window window);
  void OnWindowUnmapped(x11::Window window);

 private:
  friend struct base::DefaultSingletonTraits<GlobalMenuBarRegistrarX11>;

  GlobalMenuBarRegistrarX11();
  ~GlobalMenuBarRegistrarX11();

  // 发送实际消息。
  void RegisterXWindow(x11::Window window);
  void UnregisterXWindow(x11::Window window);

  CHROMEG_CALLBACK_1(GlobalMenuBarRegistrarX11,
                     void,
                     OnProxyCreated,
                     GObject*,
                     GAsyncResult*);
  CHROMEG_CALLBACK_1(GlobalMenuBarRegistrarX11,
                     void,
                     OnNameOwnerChanged,
                     GObject*,
                     GParamSpec*);

  GDBusProxy* registrar_proxy_ = nullptr;

  // 想要注册但尚未注册的X11：：Window，因为。
  // 我们正在等待代理服务器可用。
  std::set<x11::Window> live_windows_;

  DISALLOW_COPY_AND_ASSIGN(GlobalMenuBarRegistrarX11);
};

#endif  // SHELL_BROWSER_UI_VIEWS_GLOBAL_MENU_BAR_REGISTRAR_X11_H_
