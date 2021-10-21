// 版权所有2016年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_KIOSK_DELEGATE_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_KIOSK_DELEGATE_H_

#include "extensions/browser/kiosk/kiosk_delegate.h"
#include "extensions/common/extension_id.h"

namespace electron {

// 电子代理，提供带有Kiosk模式的扩展/应用程序API。
// 功能性。
class ElectronKioskDelegate : public extensions::KioskDelegate {
 public:
  ElectronKioskDelegate();
  ~ElectronKioskDelegate() override;

  // KioskDelegate覆盖：
  bool IsAutoLaunchedKioskApp(const extensions::ExtensionId& id) const override;
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_KIOSK_DELEGATE_H_
