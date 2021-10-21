// 版权所有2019 Slake Technologies，Inc.保留所有权利。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_PROCESS_MANAGER_DELEGATE_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_PROCESS_MANAGER_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/browser/process_manager_delegate.h"

class Browser;
class Profile;

namespace extensions {

// 支持ProcessManager。控制电子希望禁止的情况。
// 扩展背景页或推迟其创建。
class ElectronProcessManagerDelegate : public ProcessManagerDelegate {
 public:
  ElectronProcessManagerDelegate();
  ~ElectronProcessManagerDelegate() override;

  // ProcessManagerDelegate实现：
  bool AreBackgroundPagesAllowedForContext(
      content::BrowserContext* context) const override;
  bool IsExtensionBackgroundPageAllowed(
      content::BrowserContext* context,
      const Extension& extension) const override;
  bool DeferCreatingStartupBackgroundHosts(
      content::BrowserContext* context) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronProcessManagerDelegate);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_PROCESS_MANAGER_DELEGATE_H_
