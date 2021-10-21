// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_FRAME_SUBSCRIBER_H_
#define SHELL_BROWSER_API_FRAME_SUBSCRIBER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "components/viz/host/client_frame_sink_video_capturer.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "v8/include/v8.h"

namespace gfx {
class Image;
class Rect;
}  // 命名空间gfx。

namespace electron {

namespace api {

class WebContents;

class FrameSubscriber : public content::WebContentsObserver,
                        public viz::mojom::FrameSinkVideoConsumer {
 public:
  using FrameCaptureCallback =
      base::RepeatingCallback<void(const gfx::Image&, const gfx::Rect&)>;

  FrameSubscriber(content::WebContents* web_contents,
                  const FrameCaptureCallback& callback,
                  bool only_dirty);
  ~FrameSubscriber() override;

 private:
  void AttachToHost(content::RenderWidgetHost* host);
  void DetachFromHost();

  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderViewDeleted(content::RenderViewHost* host) override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;

  // VIZ：：mojom：：FrameSinkVideoConsumer实现。
  void OnFrameCaptured(
      base::ReadOnlySharedMemoryRegion data,
      ::media::mojom::VideoFrameInfoPtr info,
      const gfx::Rect& content_rect,
      mojo::PendingRemote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks>
          callbacks) override;
  void OnStopped() override;
  void OnLog(const std::string& message) override;

  void Done(const gfx::Rect& damage, const SkBitmap& frame);

  // 获取渲染视图的像素大小。
  gfx::Size GetRenderViewSize() const;

  FrameCaptureCallback callback_;
  bool only_dirty_;

  content::RenderWidgetHost* host_;
  std::unique_ptr<viz::ClientFrameSinkVideoCapturer> video_capturer_;

  base::WeakPtrFactory<FrameSubscriber> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(FrameSubscriber);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Frame_Subscriber_H_
