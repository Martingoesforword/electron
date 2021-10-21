// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_MEDIA_MEDIA_DEVICE_ID_SALT_H_
#define SHELL_BROWSER_MEDIA_MEDIA_DEVICE_ID_SALT_H_

#include <string>

#include "base/macros.h"
#include "components/prefs/pref_member.h"

class PrefRegistrySimple;
class PrefService;

namespace electron {

// MediaDeviceIDSalt负责创建和检索SALT字符串。
// 用于创建可由Web缓存的MediaSource ID。
// 服务。如果清除缓存，则MediaSourceID无效。
class MediaDeviceIDSalt {
 public:
  explicit MediaDeviceIDSalt(PrefService* pref_service);
  ~MediaDeviceIDSalt();

  std::string GetSalt();

  static void RegisterPrefs(PrefRegistrySimple* pref_registry);
  static void Reset(PrefService* pref_service);

 private:
  StringPrefMember media_device_id_salt_;

  DISALLOW_COPY_AND_ASSIGN(MediaDeviceIDSalt);
};

}  // 命名空间电子。

#endif  // Shell_Browser_MEDIA_MEDIA_DEVICE_ID_SALT_H_
