// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ZOOM_LEVEL_DELEGATE_H_
#define SHELL_BROWSER_ZOOM_LEVEL_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/zoom_level_delegate.h"

namespace base {
class DictionaryValue;
class FilePath;
}  // 命名空间库。

class PrefRegistrySimple;

namespace electron {

// 用于管理每个分区的默认缩放级别和每个主机的缩放级别的类。
// 它实现了内容/缩放之间的接口。
// HostZoomMap和首选项系统中的级别。所有更改。
// 每个分区的默认缩放级别流经此。
// 班级。当HostZoomMap调用时，对每个主机级别的任何更改都会更新。
// OnZoomLevelChanged。
class ZoomLevelDelegate : public content::ZoomLevelDelegate {
 public:
  static void RegisterPrefs(PrefRegistrySimple* pref_registry);

  ZoomLevelDelegate(PrefService* pref_service,
                    const base::FilePath& partition_path);
  ~ZoomLevelDelegate() override;

  void SetDefaultZoomLevelPref(double level);
  double GetDefaultZoomLevelPref() const;

  // 内容：：ZoomLevelDelegate：
  void InitHostZoomMap(content::HostZoomMap* host_zoom_map) override;

 private:
  void ExtractPerHostZoomLevels(
      const base::DictionaryValue* host_zoom_dictionary);

  // 这是一个从HostZoomMap接收通知的回调函数。
  // 当每个主机的缩放级别更改时。它用于更新每个主机的。
  // 此类(针对其关联分区)管理的缩放级别(如果有)。
  void OnZoomLevelChanged(const content::HostZoomMap::ZoomLevelChange& change);

  PrefService* pref_service_;
  content::HostZoomMap* host_zoom_map_ = nullptr;
  base::CallbackListSubscription zoom_subscription_;
  std::string partition_key_;

  DISALLOW_COPY_AND_ASSIGN(ZoomLevelDelegate);
};

}  // 命名空间电子。

#endif  // Shell_Browser_ZOOM_Level_Delegate_H_
