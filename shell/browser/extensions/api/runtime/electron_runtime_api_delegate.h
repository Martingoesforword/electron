// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_API_RUNTIME_ELECTRON_RUNTIME_API_DELEGATE_H_
#define SHELL_BROWSER_EXTENSIONS_API_RUNTIME_ELECTRON_RUNTIME_API_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "extensions/browser/api/runtime/runtime_api_delegate.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class ElectronRuntimeAPIDelegate : public RuntimeAPIDelegate {
 public:
  explicit ElectronRuntimeAPIDelegate(content::BrowserContext* browser_context);
  ~ElectronRuntimeAPIDelegate() override;

  // 运行APIDeleate实现。
  void AddUpdateObserver(UpdateObserver* observer) override;
  void RemoveUpdateObserver(UpdateObserver* observer) override;
  void ReloadExtension(const std::string& extension_id) override;
  bool CheckForUpdates(const std::string& extension_id,
                       UpdateCheckCallback callback) override;
  void OpenURL(const GURL& uninstall_url) override;
  bool GetPlatformInfo(api::runtime::PlatformInfo* info) override;
  bool RestartDevice(std::string* error_message) override;

 private:
  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(ElectronRuntimeAPIDelegate);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_API_RUNTIME_ELECTRON_RUNTIME_API_DELEGATE_H_
