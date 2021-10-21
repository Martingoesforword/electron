// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_WIDGET_MAC_H_
#define SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_WIDGET_MAC_H_

#include "ui/views/widget/native_widget_mac.h"

namespace electron {

class NativeWindowMac;

class ElectronNativeWidgetMac : public views::NativeWidgetMac {
 public:
  ElectronNativeWidgetMac(NativeWindowMac* shell,
                          NSUInteger style_mask,
                          views::internal::NativeWidgetDelegate* delegate);
  ~ElectronNativeWidgetMac() override;

 protected:
  // NativeWidgetMac：
  NativeWidgetMacNSWindow* CreateNSWindow(
      const remote_cocoa::mojom::CreateWindowParams* params) override;

 private:
  NativeWindowMac* shell_;
  NSUInteger style_mask_;

  DISALLOW_COPY_AND_ASSIGN(ElectronNativeWidgetMac);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_COCOA_ELECTRON_NATIVE_WIDGET_MAC_H_
