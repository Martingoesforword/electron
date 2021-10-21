// 版权所有(C)2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_UI_DEVTOOLS_MANAGER_DELEGATE_H_
#define SHELL_BROWSER_UI_DEVTOOLS_MANAGER_DELEGATE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace electron {

class DevToolsManagerDelegate : public content::DevToolsManagerDelegate {
 public:
  static void StartHttpHandler();

  DevToolsManagerDelegate();
  ~DevToolsManagerDelegate() override;

  // DevToolsManagerDelegate实现。
  void Inspect(content::DevToolsAgentHost* agent_host) override;
  void HandleCommand(content::DevToolsAgentHostClientChannel* channel,
                     base::span<const uint8_t> message,
                     NotHandledCallback callback) override;
  scoped_refptr<content::DevToolsAgentHost> CreateNewTarget(
      const GURL& url) override;
  std::string GetDiscoveryPageHTML() override;
  bool HasBundledFrontendResources() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsManagerDelegate);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_DevTools_Manager_Delegate_H_
