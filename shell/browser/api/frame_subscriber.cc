// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/frame_subscriber.h"

#include <utility>

#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "media/capture/mojom/video_capture_buffer.mojom.h"
#include "media/capture/mojom/video_capture_types.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/skbitmap_operations.h"

namespace electron {

namespace api {

constexpr static int kMaxFrameRate = 30;

FrameSubscriber::FrameSubscriber(content::WebContents* web_contents,
                                 const FrameCaptureCallback& callback,
                                 bool only_dirty)
    : content::WebContentsObserver(web_contents),
      callback_(callback),
      only_dirty_(only_dirty) {
  content::RenderViewHost* rvh = web_contents->GetRenderViewHost();
  if (rvh)
    AttachToHost(rvh->GetWidget());
}

FrameSubscriber::~FrameSubscriber() = default;

void FrameSubscriber::AttachToHost(content::RenderWidgetHost* host) {
  host_ = host;

  // 如果呈现器进程已崩溃，则该视图可能为空。
  // (https://crbug.com/847363))。
  if (!host_->GetView())
    return;

  // 创建并配置视频采集器。
  gfx::Size size = GetRenderViewSize();
  video_capturer_ = host_->GetView()->CreateVideoCapturer();
  video_capturer_->SetResolutionConstraints(size, size, true);
  video_capturer_->SetAutoThrottlingEnabled(false);
  video_capturer_->SetMinSizeChangePeriod(base::TimeDelta());
  video_capturer_->SetFormat(media::PIXEL_FORMAT_ARGB,
                             gfx::ColorSpace::CreateREC709());
  video_capturer_->SetMinCapturePeriod(base::TimeDelta::FromSeconds(1) /
                                       kMaxFrameRate);
  video_capturer_->Start(this);
}

void FrameSubscriber::DetachFromHost() {
  if (!host_)
    return;
  video_capturer_.reset();
  host_ = nullptr;
}

void FrameSubscriber::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  if (!host_)
    AttachToHost(render_frame_host->GetRenderWidgetHost());
}

void FrameSubscriber::RenderViewDeleted(content::RenderViewHost* host) {
  if (host->GetWidget() == host_) {
    DetachFromHost();
  }
}

void FrameSubscriber::RenderViewHostChanged(content::RenderViewHost* old_host,
                                            content::RenderViewHost* new_host) {
  if ((old_host && old_host->GetWidget() == host_) || (!old_host && !host_)) {
    DetachFromHost();
    AttachToHost(new_host->GetWidget());
  }
}

void FrameSubscriber::OnFrameCaptured(
    base::ReadOnlySharedMemoryRegion data,
    ::media::mojom::VideoFrameInfoPtr info,
    const gfx::Rect& content_rect,
    mojo::PendingRemote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks>
        callbacks) {
  gfx::Size size = GetRenderViewSize();
  if (size != content_rect.size()) {
    video_capturer_->SetResolutionConstraints(size, size, true);
    video_capturer_->RequestRefreshFrame();
    return;
  }

  mojo::Remote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks>
      callbacks_remote(std::move(callbacks));
  if (!data.IsValid()) {
    callbacks_remote->Done();
    return;
  }
  base::ReadOnlySharedMemoryMapping mapping = data.Map();
  if (!mapping.IsValid()) {
    DLOG(ERROR) << "Shared memory mapping failed.";
    return;
  }
  if (mapping.size() <
      media::VideoFrame::AllocationSize(info->pixel_format, info->coded_size)) {
    DLOG(ERROR) << "Shared memory size was less than expected.";
    return;
  }

  // SkBitmap的像素将标记为不可变，但installPixels()。
  // API需要非常数指针。所以，扔掉这位君主吧。
  void* const pixels = const_cast<void*>(mapping.memory());

  // 使用|RelaseProc|调用installPixels()：1)通知捕捉器。
  // 该使用者已经完成了帧，并且2)释放共享的。
  // 内存映射。
  struct FramePinner {
    // 保持支持|FRAME_|映射的共享内存。
    base::ReadOnlySharedMemoryMapping mapping;
    // 防止FrameSinkVideoCapturer回收共享内存。
    // BACKS|FRAME_|。
    mojo::Remote<viz::mojom::FrameSinkVideoConsumerFrameCallbacks> releaser;
  };

  SkBitmap bitmap;
  bitmap.installPixels(
      SkImageInfo::MakeN32(content_rect.width(), content_rect.height(),
                           kPremul_SkAlphaType),
      pixels,
      media::VideoFrame::RowBytes(media::VideoFrame::kARGBPlane,
                                  info->pixel_format, info->coded_size.width()),
      [](void* addr, void* context) {
        delete static_cast<FramePinner*>(context);
      },
      new FramePinner{std::move(mapping), std::move(callbacks_remote)});
  bitmap.setImmutable();

  Done(content_rect, bitmap);
}

void FrameSubscriber::OnStopped() {}

void FrameSubscriber::OnLog(const std::string& message) {}

void FrameSubscriber::Done(const gfx::Rect& damage, const SkBitmap& frame) {
  if (frame.drawsNothing())
    return;

  const SkBitmap& bitmap = only_dirty_ ? SkBitmapOperations::CreateTiledBitmap(
                                             frame, damage.x(), damage.y(),
                                             damage.width(), damage.height())
                                       : frame;

  // 复制SkBitmap不会复制内部像素，我们必须手动。
  // 分配和写入像素，否则当原始。
  // 框架已修改。
  SkBitmap copy;
  copy.allocPixels(SkImageInfo::Make(bitmap.width(), bitmap.height(),
                                     kN32_SkColorType, kPremul_SkAlphaType));
  SkPixmap pixmap;
  bool success = bitmap.peekPixels(&pixmap) && copy.writePixels(pixmap, 0, 0);
  CHECK(success);

  callback_.Run(gfx::Image::CreateFrom1xBitmap(copy), damage);
}

gfx::Size FrameSubscriber::GetRenderViewSize() const {
  content::RenderWidgetHostView* view = host_->GetView();
  gfx::Size size = view->GetViewBounds().size();
  return gfx::ToRoundedSize(
      gfx::ScaleSize(gfx::SizeF(size), view->GetDeviceScaleFactor()));
}

}  // 命名空间API。

}  // 命名空间电子
