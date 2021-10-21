// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/extensions/electron_extensions_renderer_client.h"

#include <string>

#include "content/public/renderer/render_thread.h"
#include "extensions/common/constants.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/renderer/dispatcher.h"
#include "shell/common/world_ids.h"
#include "shell/renderer/extensions/electron_extensions_dispatcher_delegate.h"

namespace electron {

ElectronExtensionsRendererClient::ElectronExtensionsRendererClient()
    : dispatcher_(std::make_unique<extensions::Dispatcher>(
          std::make_unique<ElectronExtensionsDispatcherDelegate>())) {
  dispatcher_->OnRenderThreadStarted(content::RenderThread::Get());
}

ElectronExtensionsRendererClient::~ElectronExtensionsRendererClient() = default;

bool ElectronExtensionsRendererClient::IsIncognitoProcess() const {
  // APP_SHELL不支持非记录上下文。
  return false;
}

int ElectronExtensionsRendererClient::GetLowestIsolatedWorldId() const {
  return WorldIDs::ISOLATED_WORLD_ID_EXTENSIONS;
}

extensions::Dispatcher* ElectronExtensionsRendererClient::GetDispatcher() {
  return dispatcher_.get();
}

bool ElectronExtensionsRendererClient::
    ExtensionAPIEnabledForServiceWorkerScript(const GURL& scope,
                                              const GURL& script_url) const {
  if (!script_url.SchemeIs(extensions::kExtensionScheme))
    return false;

  const extensions::Extension* extension =
      extensions::RendererExtensionRegistry::Get()->GetExtensionOrAppByURL(
          script_url);

  if (!extension ||
      !extensions::BackgroundInfo::IsServiceWorkerBased(extension))
    return false;

  if (scope != extension->url())
    return false;

  const std::string& sw_script =
      extensions::BackgroundInfo::GetBackgroundServiceWorkerScript(extension);

  return extension->GetResourceURL(sw_script) == script_url;
}

bool ElectronExtensionsRendererClient::AllowPopup() {
  // TODO(Samuelmaddock)：
  return false;
}

void ElectronExtensionsRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  dispatcher_->RunScriptsAtDocumentStart(render_frame);
}

void ElectronExtensionsRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  dispatcher_->RunScriptsAtDocumentEnd(render_frame);
}

void ElectronExtensionsRendererClient::RunScriptsAtDocumentIdle(
    content::RenderFrame* render_frame) {
  dispatcher_->RunScriptsAtDocumentIdle(render_frame);
}

}  // 命名空间电子
