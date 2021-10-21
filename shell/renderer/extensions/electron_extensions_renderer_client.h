// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_RENDERER_CLIENT_H_
#define SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_RENDERER_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "extensions/renderer/extensions_renderer_client.h"

namespace content {
class RenderFrame;
}

namespace extensions {
class Dispatcher;
}

namespace electron {

class ElectronExtensionsRendererClient
    : public extensions::ExtensionsRendererClient {
 public:
  ElectronExtensionsRendererClient();
  ~ElectronExtensionsRendererClient() override;

  // ExtensionsRendererClient实现。
  bool IsIncognitoProcess() const override;
  int GetLowestIsolatedWorldId() const override;
  extensions::Dispatcher* GetDispatcher() override;
  bool ExtensionAPIEnabledForServiceWorkerScript(
      const GURL& scope,
      const GURL& script_url) const override;

  bool AllowPopup();

  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame);
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame);
  void RunScriptsAtDocumentIdle(content::RenderFrame* render_frame);

 private:
  std::unique_ptr<extensions::Dispatcher> dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionsRendererClient);
};

}  // 命名空间电子。

#endif  // SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_RENDERER_CLIENT_H_
