// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_OSR_OSR_VIDEO_CONSUMER_H_
#define SHELL_BROWSER_OSR_OSR_VIDEO_CONSUMER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "components/viz/host/client_frame_sink_video_capturer.h"
#include "media/capture/mojom/video_capture_types.mojom.h"

namespace electron {

class OffScreenRenderWidgetHostView;

typedef base::RepeatingCallback<void(const gfx::Rect&, const SkBitmap&)>
    OnPaintCallback;

class OffScreenVideoConsumer : public viz::mojom::FrameSinkVideoConsumer {
 public:
  OffScreenVideoConsumer(OffScreenRenderWidgetHostView* view,
                         OnPaintCallback callback);
  ~OffScreenVideoConsumer() override;

  void SetActive(bool active);
  void SetFrameRate(int frame_rate);
  void SizeChanged();

 private:
  // VIZ：：mojom：：FrameSinkVideoConsumer实现。
  void OnFrameCaptured(
      base::ReadOnlySharedMemoryRegion data,
      ::media::mojom::VideoFrameInfoPtr info,
      const gfx::Rect& content_rect,
      mojo::PendingRemote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks>
          callbacks) override;
  void OnStopped() override;
  void OnLog(const std::string& message) override;

  bool CheckContentRect(const gfx::Rect& content_rect);

  OnPaintCallback callback_;

  OffScreenRenderWidgetHostView* view_;
  std::unique_ptr<viz::ClientFrameSinkVideoCapturer> video_capturer_;

  base::WeakPtrFactory<OffScreenVideoConsumer> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(OffScreenVideoConsumer);
};

}  // 命名空间电子。

#endif  // Shell_Browser_OSR_OSR_VIDEO_Consumer_H_
