// 版权所有(C)2020 Samuel Maddock&lt;Sam@samuelmaddock.com&gt;。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_
#define SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/process/process.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/pinnable.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom-forward.h"

class GURL;

namespace content {
class RenderFrameHost;
}

namespace gin {
class Arguments;
}

namespace electron {

namespace api {

class WebContents;

// 用于从主进程访问帧的绑定。
class WebFrameMain : public gin::Wrappable<WebFrameMain>,
                     public gin_helper::EventEmitterMixin<WebFrameMain>,
                     public gin_helper::Pinnable<WebFrameMain>,
                     public gin_helper::Constructible<WebFrameMain> {
 public:
  // 创建一个新的WebFrameMain，并返回它的V8包装器。
  static gin::Handle<WebFrameMain> New(v8::Isolate* isolate);

  static gin::Handle<WebFrameMain> From(
      v8::Isolate* isolate,
      content::RenderFrameHost* render_frame_host);
  static WebFrameMain* FromFrameTreeNodeId(int frame_tree_node_id);
  static WebFrameMain* FromRenderFrameHost(
      content::RenderFrameHost* render_frame_host);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
      v8::Isolate*,
      v8::Local<v8::ObjectTemplate>);
  const char* GetTypeName() override;

  content::RenderFrameHost* render_frame_host() const { return render_frame_; }

 protected:
  explicit WebFrameMain(content::RenderFrameHost* render_frame);
  ~WebFrameMain() override;

 private:
  friend class WebContents;

  // 在删除FrameTreeNode时调用。
  void Destroyed();

  // 将RenderFrameHost标记为已释放并不再访问它。这可以。
  // 在WebFrameMain V8句柄被GC或FrameTreeNode时发生。
  // 被移除。
  void MarkRenderFrameDisposed();

  // 跨域导航时换出内部RFH。
  void UpdateRenderFrameHost(content::RenderFrameHost* rfh);

  const mojo::Remote<mojom::ElectronRenderer>& GetRendererApi();

  // WebFrameMain的生存期可能超过其RenderFrameHost指针，因此我们需要检查。
  // 在访问它之前是否已被处理。
  bool CheckRenderFrame() const;

  v8::Local<v8::Promise> ExecuteJavaScript(gin::Arguments* args,
                                           const std::u16string& code);
  bool Reload();
  void Send(v8::Isolate* isolate,
            bool internal,
            const std::string& channel,
            v8::Local<v8::Value> args);
  void PostMessage(v8::Isolate* isolate,
                   const std::string& channel,
                   v8::Local<v8::Value> message_value,
                   absl::optional<v8::Local<v8::Value>> transfer);

  int FrameTreeNodeID() const;
  std::string Name() const;
  base::ProcessId OSProcessID() const;
  int ProcessID() const;
  int RoutingID() const;
  GURL URL() const;
  blink::mojom::PageVisibilityState VisibilityState() const;

  content::RenderFrameHost* Top() const;
  content::RenderFrameHost* Parent() const;
  std::vector<content::RenderFrameHost*> Frames() const;
  std::vector<content::RenderFrameHost*> FramesInSubtree() const;

  void OnRendererConnectionError();
  void Connect();
  void DOMContentLoaded();

  mojo::Remote<mojom::ElectronRenderer> renderer_api_;
  mojo::PendingReceiver<mojom::ElectronRenderer> pending_receiver_;

  int frame_tree_node_id_;

  content::RenderFrameHost* render_frame_ = nullptr;

  // RenderFrameHost是否已删除以及不应再。
  // 被访问。
  bool render_frame_disposed_ = false;

  base::WeakPtrFactory<WebFrameMain> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(WebFrameMain);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_WEB_FRAME_MAIN_H_
