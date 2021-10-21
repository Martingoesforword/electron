// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_
#define SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_

#include <string>

#include "content/public/renderer/render_frame_observer.h"
#include "ipc/ipc_platform_file.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

class RendererClientBase;

// Helper类将消息转发到客户端。
class ElectronRenderFrameObserver : public content::RenderFrameObserver {
 public:
  ElectronRenderFrameObserver(content::RenderFrame* frame,
                              RendererClientBase* renderer_client);

  // 内容：：RenderFrameWatch：
  void DidClearWindowObject() override;
  void DidInstallConditionalFeatures(v8::Handle<v8::Context> context,
                                     int world_id) override;
  void DraggableRegionsChanged() override;
  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                int world_id) override;
  void OnDestruct() override;
  void DidMeaningfulLayout(blink::WebMeaningfulLayout layout_type) override;

 private:
  bool ShouldNotifyClient(int world_id);
  void CreateIsolatedWorldContext();
  bool IsMainWorld(int world_id);
  bool IsIsolatedWorld(int world_id);
  void OnTakeHeapSnapshot(IPC::PlatformFileForTransit file_handle,
                          const std::string& channel);

  content::RenderFrame* render_frame_;
  RendererClientBase* renderer_client_;

  DISALLOW_COPY_AND_ASSIGN(ElectronRenderFrameObserver);
};

}  // 命名空间电子。

#endif  // SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_
