// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_DISPLAY_INFO_PROVIDER_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_DISPLAY_INFO_PROVIDER_H_

#include "base/macros.h"
#include "extensions/browser/api/system_display/display_info_provider.h"

namespace extensions {

class ElectronDisplayInfoProvider : public DisplayInfoProvider {
 public:
  ElectronDisplayInfoProvider();

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronDisplayInfoProvider);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_DISPLAY_INFO_PROVIDER_H_
