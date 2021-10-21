// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/media/media_device_id_salt.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace electron {

namespace {

const char kMediaDeviceIdSalt[] = "electron.media.device_id_salt";

}  // 命名空间。

MediaDeviceIDSalt::MediaDeviceIDSalt(PrefService* pref_service) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  media_device_id_salt_.Init(kMediaDeviceIdSalt, pref_service);
  if (media_device_id_salt_.GetValue().empty()) {
    media_device_id_salt_.SetValue(
        content::BrowserContext::CreateRandomMediaDeviceIDSalt());
  }
}

MediaDeviceIDSalt::~MediaDeviceIDSalt() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  media_device_id_salt_.Destroy();
}

std::string MediaDeviceIDSalt::GetSalt() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return media_device_id_salt_.GetValue();
}

// 静电。
void MediaDeviceIDSalt::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kMediaDeviceIdSalt, std::string());
}

// 静电。
void MediaDeviceIDSalt::Reset(PrefService* pref_service) {
  pref_service->SetString(
      kMediaDeviceIdSalt,
      content::BrowserContext::CreateRandomMediaDeviceIDSalt());
}

}  // 命名空间电子
