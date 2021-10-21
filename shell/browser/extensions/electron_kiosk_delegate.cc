// 版权所有2016年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/electron_kiosk_delegate.h"

namespace electron {

ElectronKioskDelegate::ElectronKioskDelegate() = default;

ElectronKioskDelegate::~ElectronKioskDelegate() = default;

bool ElectronKioskDelegate::IsAutoLaunchedKioskApp(
    const extensions::ExtensionId& id) const {
  return false;
}

}  // 命名空间电子
