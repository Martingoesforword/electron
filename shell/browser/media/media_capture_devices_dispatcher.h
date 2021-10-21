// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
#define SHELL_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_

#include <string>

#include "base/memory/singleton.h"
#include "content/public/browser/media_observer.h"
#include "content/public/browser/media_stream_request.h"
#include "third_party/blink/public/common/mediastream/media_stream_request.h"

namespace electron {

// 此单例用于从内容接收有关媒体事件的更新。
// 一层。
class MediaCaptureDevicesDispatcher : public content::MediaObserver {
 public:
  static MediaCaptureDevicesDispatcher* GetInstance();

  // 观察者的方法。在UI线程上调用。
  const blink::MediaStreamDevices& GetAudioCaptureDevices();
  const blink::MediaStreamDevices& GetVideoCaptureDevices();

  // Helper，获取媒体请求可以使用的默认设备。
  // 如果默认设备不可用，则使用第一个可用设备。
  // 如果返回列表为空，则表示。
  // 奥斯。
  // 在UI线程上调用。
  void GetDefaultDevices(bool audio,
                         bool video,
                         blink::MediaStreamDevices* devices);

  // 用于挑选特定请求设备的帮助器，由原始ID标识。
  // 如果请求的设备不可用，它将返回NULL。
  const blink::MediaStreamDevice* GetRequestedAudioDevice(
      const std::string& requested_audio_device_id);
  const blink::MediaStreamDevice* GetRequestedVideoDevice(
      const std::string& requested_video_device_id);

  // 返回第一个可用的音频或视频设备，如果没有设备，则返回NULL。
  // 都是可用的。
  const blink::MediaStreamDevice* GetFirstAvailableAudioDevice();
  const blink::MediaStreamDevice* GetFirstAvailableVideoDevice();

  // 不需要实际设备枚举的单元测试应调用此。
  // 单例上的API。方法多次调用此函数是安全的。
  // 辛格尔顿。
  void DisableDeviceEnumerationForTesting();

  // 从Content：：MediaViewer覆盖：
  void OnAudioCaptureDevicesChanged() override;
  void OnVideoCaptureDevicesChanged() override;
  void OnMediaRequestStateChanged(int render_process_id,
                                  int render_view_id,
                                  int page_request_id,
                                  const GURL& security_origin,
                                  blink::mojom::MediaStreamType stream_type,
                                  content::MediaRequestState state) override;
  void OnCreatingAudioStream(int render_process_id,
                             int render_view_id) override;
  void OnSetCapturingLinkSecured(int render_process_id,
                                 int render_frame_id,
                                 int page_request_id,
                                 blink::mojom::MediaStreamType stream_type,
                                 bool is_secure) override;

 private:
  friend struct base::DefaultSingletonTraits<MediaCaptureDevicesDispatcher>;

  MediaCaptureDevicesDispatcher();
  ~MediaCaptureDevicesDispatcher() override;

  // 仅用于测试，缓存的音频捕获设备列表。
  blink::MediaStreamDevices test_audio_devices_;

  // 仅用于测试，缓存的视频捕获设备列表。
  blink::MediaStreamDevices test_video_devices_;

  // 单元测试用来禁用设备枚举的标志。
  bool is_device_enumeration_disabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(MediaCaptureDevicesDispatcher);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_MEDIA_MEDIA_CAPTURE_DEVICES_DISPATCHER_H_
