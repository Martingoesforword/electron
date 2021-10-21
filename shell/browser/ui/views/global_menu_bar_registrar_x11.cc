// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/views/global_menu_bar_registrar_x11.h"

#include <string>

#include "base/bind.h"
#include "base/debug/leak_annotations.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/ui/views/global_menu_bar_x11.h"

using content::BrowserThread;

namespace {

const char kAppMenuRegistrarName[] = "com.canonical.AppMenu.Registrar";
const char kAppMenuRegistrarPath[] = "/com/canonical/AppMenu/Registrar";

}  // 命名空间。

// 静电。
GlobalMenuBarRegistrarX11* GlobalMenuBarRegistrarX11::GetInstance() {
  return base::Singleton<GlobalMenuBarRegistrarX11>::get();
}

void GlobalMenuBarRegistrarX11::OnWindowMapped(x11::Window window) {
  live_windows_.insert(window);

  if (registrar_proxy_)
    RegisterXWindow(window);
}

void GlobalMenuBarRegistrarX11::OnWindowUnmapped(x11::Window window) {
  if (registrar_proxy_)
    UnregisterXWindow(window);

  live_windows_.erase(window);
}

GlobalMenuBarRegistrarX11::GlobalMenuBarRegistrarX11() {
  // LibdbusMenu使用的是dbus的gio版本；我尝试使用dbus/中的代码，
  // 但看起来它并没有和gio版本共享公交车的名字，
  // 即使在|CONNECTION_TYPE|设置为SHARED时也是如此。
  g_dbus_proxy_new_for_bus(
      G_BUS_TYPE_SESSION,
      static_cast<GDBusProxyFlags>(G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                                   G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS |
                                   G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START),
      nullptr, kAppMenuRegistrarName, kAppMenuRegistrarPath,
      kAppMenuRegistrarName,
      nullptr,  // 可能想要一个真正的可取消的。
      static_cast<GAsyncReadyCallback>(OnProxyCreatedThunk), this);
}

GlobalMenuBarRegistrarX11::~GlobalMenuBarRegistrarX11() {
  if (registrar_proxy_) {
    g_signal_handlers_disconnect_by_func(
        registrar_proxy_, reinterpret_cast<void*>(OnNameOwnerChangedThunk),
        this);
    g_object_unref(registrar_proxy_);
  }
}

void GlobalMenuBarRegistrarX11::RegisterXWindow(x11::Window window) {
  DCHECK(registrar_proxy_);
  std::string path = electron::GlobalMenuBarX11::GetPathForWindow(window);

  ANNOTATE_SCOPED_MEMORY_LEAK;  // Http://crbug.com/314087。
  // TODO(Erg)：Mozilla实现遇到了很多回调麻烦。
  // 只是为了确保他们做出反应，确保有某种。
  // 对象；包括进行整个回调来处理。
  // 可以取消的。
  // 
  // 我看不出我们为什么要关心“RegisterWindow”是否完成或。
  // 不。
  g_dbus_proxy_call(registrar_proxy_, "RegisterWindow",
                    g_variant_new("(uo)", window, path.c_str()),
                    G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
}

void GlobalMenuBarRegistrarX11::UnregisterXWindow(x11::Window window) {
  DCHECK(registrar_proxy_);
  std::string path = electron::GlobalMenuBarX11::GetPathForWindow(window);

  ANNOTATE_SCOPED_MEMORY_LEAK;  // Http://crbug.com/314087。
  // TODO(Erg)：Mozilla实现遇到了很多回调麻烦。
  // 只是为了确保他们做出反应，确保有某种。
  // 对象；包括进行整个回调来处理。
  // 可以取消的。
  // 
  // 我看不出我们为什么要关心“UnregisterWindow”是否完成。
  // 或者不去。
  g_dbus_proxy_call(registrar_proxy_, "UnregisterWindow",
                    g_variant_new("(u)", window), G_DBUS_CALL_FLAGS_NONE, -1,
                    nullptr, nullptr, nullptr);
}

void GlobalMenuBarRegistrarX11::OnProxyCreated(GObject* source,
                                               GAsyncResult* result) {
  GError* error = nullptr;
  GDBusProxy* proxy = g_dbus_proxy_new_for_bus_finish(result, &error);
  if (error) {
    g_error_free(error);
    return;
  }

  // TODO(Erg)：Mozilla的实现可以解决GDBus问题。
  // 在这里取消。但是，它被标记为已修复。如果有什么奇怪的。
  // 取消的问题，看看他们是如何解决问题的。

  registrar_proxy_ = proxy;

  g_signal_connect(registrar_proxy_, "notify::g-name-owner",
                   G_CALLBACK(OnNameOwnerChangedThunk), this);

  OnNameOwnerChanged(nullptr, nullptr);
}

void GlobalMenuBarRegistrarX11::OnNameOwnerChanged(GObject* /* 忽略。*/,
                                                   GParamSpec* /* 忽略。*/) {
  // 如果名称Owner更改，我们需要重新注册所有活动的x11：：Window。
  // 与这个系统的关系。
  for (const auto& window : live_windows_) {
    RegisterXWindow(window);
  }
}
