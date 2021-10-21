// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_BROWSER_HANDLER_IMPL_H_
#define SHELL_BROWSER_ELECTRON_BROWSER_HANDLER_IMPL_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "electron/shell/common/api/api.mojom.h"
#include "shell/browser/api/electron_api_web_contents.h"

namespace content {
class RenderFrameHost;
}

namespace electron {
class ElectronBrowserHandlerImpl : public mojom::ElectronBrowser,
                                   public content::WebContentsObserver {
 public:
  explicit ElectronBrowserHandlerImpl(
      content::RenderFrameHost* render_frame_host,
      mojo::PendingReceiver<mojom::ElectronBrowser> receiver);

  static void Create(content::RenderFrameHost* frame_host,
                     mojo::PendingReceiver<mojom::ElectronBrowser> receiver);

  // Mojom：：ElectronBrowser：
  void Message(bool internal,
               const std::string& channel,
               blink::CloneableMessage arguments) override;
  void Invoke(bool internal,
              const std::string& channel,
              blink::CloneableMessage arguments,
              InvokeCallback callback) override;
  void OnFirstNonEmptyLayout() override;
  void ReceivePostMessage(const std::string& channel,
                          blink::TransferableMessage message) override;
  void MessageSync(bool internal,
                   const std::string& channel,
                   blink::CloneableMessage arguments,
                   MessageSyncCallback callback) override;
  void MessageTo(int32_t web_contents_id,
                 const std::string& channel,
                 blink::CloneableMessage arguments) override;
  void MessageHost(const std::string& channel,
                   blink::CloneableMessage arguments) override;
  void UpdateDraggableRegions(
      std::vector<mojom::DraggableRegionPtr> regions) override;
  void SetTemporaryZoomLevel(double level) override;
  void DoGetZoomLevel(DoGetZoomLevelCallback callback) override;

  base::WeakPtr<ElectronBrowserHandlerImpl> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  ~ElectronBrowserHandlerImpl() override;

  // 内容：：WebContentsViewer：
  void WebContentsDestroyed() override;

  void OnConnectionError();

  content::RenderFrameHost* GetRenderFrameHost();

  const int render_process_id_;
  const int render_frame_id_;

  mojo::Receiver<mojom::ElectronBrowser> receiver_{this};

  base::WeakPtrFactory<ElectronBrowserHandlerImpl> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ElectronBrowserHandlerImpl);
};
}  // 命名空间电子。
#endif  // SHELL_BROWSER_ELECTRON_BROWSER_HANDLER_IMPL_H_
