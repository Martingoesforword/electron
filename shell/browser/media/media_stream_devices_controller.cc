// 版权所有(C)2012 Chromium作者。保留所有权利。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#include "shell/browser/media/media_stream_devices_controller.h"

#include <memory>
#include <utility>

#include "content/public/browser/desktop_media_id.h"
#include "content/public/browser/media_stream_request.h"
#include "shell/browser/media/media_capture_devices_dispatcher.h"

namespace electron {

namespace {

bool HasAnyAvailableDevice() {
  const blink::MediaStreamDevices& audio_devices =
      MediaCaptureDevicesDispatcher::GetInstance()->GetAudioCaptureDevices();
  const blink::MediaStreamDevices& video_devices =
      MediaCaptureDevicesDispatcher::GetInstance()->GetVideoCaptureDevices();

  return !audio_devices.empty() || !video_devices.empty();
}

}  // 命名空间。

MediaStreamDevicesController::MediaStreamDevicesController(
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback)
    : request_(request),
      callback_(std::move(callback)),
      // 对于media_open_device请求(Pepper)，我们总是同时请求两个网络摄像头。
      // 和麦克风，以避免弹出两个信息杆。
      microphone_requested_(
          request.audio_type ==
              blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE ||
          request.request_type == blink::MEDIA_OPEN_DEVICE_PEPPER_ONLY),
      webcam_requested_(
          request.video_type ==
              blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE ||
          request.request_type == blink::MEDIA_OPEN_DEVICE_PEPPER_ONLY) {}

MediaStreamDevicesController::~MediaStreamDevicesController() {
  if (!callback_.is_null()) {
    std::move(callback_).Run(
        blink::MediaStreamDevices(),
        blink::mojom::MediaStreamRequestResult::FAILED_DUE_TO_SHUTDOWN,
        std::unique_ptr<content::MediaStreamUI>());
  }
}

bool MediaStreamDevicesController::TakeAction() {
  // 对桌面屏幕投射进行特殊处理。
  if (request_.audio_type ==
          blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE ||
      request_.video_type ==
          blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE ||
      request_.audio_type ==
          blink::mojom::MediaStreamType::GUM_DESKTOP_AUDIO_CAPTURE ||
      request_.video_type ==
          blink::mojom::MediaStreamType::GUM_DESKTOP_VIDEO_CAPTURE) {
    HandleUserMediaRequest();
    return true;
  }

  // 如果没有设备连接到操作系统，则拒绝请求。
  if (!HasAnyAvailableDevice()) {
    Deny(blink::mojom::MediaStreamRequestResult::NO_HARDWARE);
    return true;
  }

  Accept();
  return true;
}

void MediaStreamDevicesController::Accept() {
  // 获取请求的默认设备。
  blink::MediaStreamDevices devices;
  if (microphone_requested_ || webcam_requested_) {
    switch (request_.request_type) {
      case blink::MEDIA_OPEN_DEVICE_PEPPER_ONLY: {
        const blink::MediaStreamDevice* device = nullptr;
        // 对于打开的设备请求，请选择所需的设备或回退到。
        // 给定类型的第一个可用的。
        if (request_.audio_type ==
            blink::mojom::MediaStreamType::DEVICE_AUDIO_CAPTURE) {
          device =
              MediaCaptureDevicesDispatcher::GetInstance()
                  ->GetRequestedAudioDevice(request_.requested_audio_device_id);
          // TODO(Wjia)：确认这是预期的行为。
          if (!device) {
            device = MediaCaptureDevicesDispatcher::GetInstance()
                         ->GetFirstAvailableAudioDevice();
          }
        } else if (request_.video_type ==
                   blink::mojom::MediaStreamType::DEVICE_VIDEO_CAPTURE) {
          // Pepper API一次只能打开一个设备。
          device =
              MediaCaptureDevicesDispatcher::GetInstance()
                  ->GetRequestedVideoDevice(request_.requested_video_device_id);
          // TODO(Wjia)：确认这是预期的行为。
          if (!device) {
            device = MediaCaptureDevicesDispatcher::GetInstance()
                         ->GetFirstAvailableVideoDevice();
          }
        }
        if (device)
          devices.push_back(*device);
        break;
      }
      case blink::MEDIA_GENERATE_STREAM: {
        bool needs_audio_device = microphone_requested_;
        bool needs_video_device = webcam_requested_;

        // 如果指定了ID，则获取确切的音频或视频设备。
        if (!request_.requested_audio_device_id.empty()) {
          const blink::MediaStreamDevice* audio_device =
              MediaCaptureDevicesDispatcher::GetInstance()
                  ->GetRequestedAudioDevice(request_.requested_audio_device_id);
          if (audio_device) {
            devices.push_back(*audio_device);
            needs_audio_device = false;
          }
        }
        if (!request_.requested_video_device_id.empty()) {
          const blink::MediaStreamDevice* video_device =
              MediaCaptureDevicesDispatcher::GetInstance()
                  ->GetRequestedVideoDevice(request_.requested_video_device_id);
          if (video_device) {
            devices.push_back(*video_device);
            needs_video_device = false;
          }
        }

        // 如果请求了音频和/或视频设备中的一个或两个，但未请求。
        // 由id指定，获取默认设备。
        if (needs_audio_device || needs_video_device) {
          MediaCaptureDevicesDispatcher::GetInstance()->GetDefaultDevices(
              needs_audio_device, needs_video_device, &devices);
        }
        break;
      }
      case blink::MEDIA_DEVICE_ACCESS: {
        // 获取请求的默认设备。
        MediaCaptureDevicesDispatcher::GetInstance()->GetDefaultDevices(
            microphone_requested_, webcam_requested_, &devices);
        break;
      }
      case blink::MEDIA_DEVICE_UPDATE: {
        NOTREACHED();
        break;
      }
    }
  }

  std::move(callback_).Run(devices, blink::mojom::MediaStreamRequestResult::OK,
                           std::unique_ptr<content::MediaStreamUI>());
}

void MediaStreamDevicesController::Deny(
    blink::mojom::MediaStreamRequestResult result) {
  std::move(callback_).Run(blink::MediaStreamDevices(), result,
                           std::unique_ptr<content::MediaStreamUI>());
}

void MediaStreamDevicesController::HandleUserMediaRequest() {
  blink::MediaStreamDevices devices;

  if (request_.audio_type ==
      blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE) {
    devices.emplace_back(blink::mojom::MediaStreamType::GUM_TAB_AUDIO_CAPTURE,
                         "", "");
  }
  if (request_.video_type ==
      blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE) {
    devices.emplace_back(blink::mojom::MediaStreamType::GUM_TAB_VIDEO_CAPTURE,
                         "", "");
  }
  if (request_.audio_type ==
      blink::mojom::MediaStreamType::GUM_DESKTOP_AUDIO_CAPTURE) {
    devices.emplace_back(
        blink::mojom::MediaStreamType::GUM_DESKTOP_AUDIO_CAPTURE, "loopback",
        "System Audio");
  }
  if (request_.video_type ==
      blink::mojom::MediaStreamType::GUM_DESKTOP_VIDEO_CAPTURE) {
    content::DesktopMediaID screen_id;
    // 如果未指定设备ID，则这是一个屏幕截图请求。
    // (即没有使用chooseDesktopMedia()接口生成设备ID)。
    if (request_.requested_video_device_id.empty()) {
      screen_id = content::DesktopMediaID(content::DesktopMediaID::TYPE_SCREEN,
                                          -1 /* KFullDesktopScreenId。*/);
    } else {
      screen_id =
          content::DesktopMediaID::Parse(request_.requested_video_device_id);
    }

    devices.emplace_back(
        blink::mojom::MediaStreamType::GUM_DESKTOP_VIDEO_CAPTURE,
        screen_id.ToString(), "Screen");
  }

  std::move(callback_).Run(
      devices,
      devices.empty() ? blink::mojom::MediaStreamRequestResult::NO_HARDWARE
                      : blink::mojom::MediaStreamRequestResult::OK,
      std::unique_ptr<content::MediaStreamUI>());
}

}  // 命名空间电子
