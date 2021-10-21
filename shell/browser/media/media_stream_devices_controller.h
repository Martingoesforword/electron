// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_MEDIA_MEDIA_STREAM_DEVICES_CONTROLLER_H_
#define SHELL_BROWSER_MEDIA_MEDIA_STREAM_DEVICES_CONTROLLER_H_

#include "content/public/browser/web_contents_delegate.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"

namespace electron {

class MediaStreamDevicesController {
 public:
  MediaStreamDevicesController(const content::MediaStreamRequest& request,
                               content::MediaResponseCallback callback);

  virtual ~MediaStreamDevicesController();

  // 根据默认策略接受或拒绝请求。
  bool TakeAction();

  // 明确接受或拒绝该请求。
  void Accept();
  void Deny(blink::mojom::MediaStreamRequestResult result);

 private:
  // 处理桌面或选项卡式投屏请求。
  void HandleUserMediaRequest();

  // 访问设备的原始请求。
  const content::MediaStreamRequest request_;

  // 需要运行以通知WebRTC是否访问。
  // 是否授予音频/视频设备。
  content::MediaResponseCallback callback_;

  bool microphone_requested_;
  bool webcam_requested_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamDevicesController);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_MEDIA_MEDIA_STREAM_DEVICES_CONTROLLER_H_
