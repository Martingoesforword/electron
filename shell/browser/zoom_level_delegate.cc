// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/zoom_level_delegate.h"

#include <functional>
#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/common/page/page_zoom.h"

namespace electron {

namespace {

// 双精度，表示默认缩放级别。
const char kPartitionDefaultZoomLevel[] = "partition.default_zoom_level";

// 将主机名映射到缩放级别的字典。不在此首选项中主机将。
// 以默认缩放级别显示。
const char kPartitionPerHostZoomLevels[] = "partition.per_host_zoom_levels";

std::string GetHash(const base::FilePath& partition_path) {
  size_t int_key = std::hash<base::FilePath>()(partition_path);
  return base::NumberToString(int_key);
}

}  // 命名空间。

// 静电。
void ZoomLevelDelegate::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kPartitionDefaultZoomLevel);
  registry->RegisterDictionaryPref(kPartitionPerHostZoomLevels);
}

ZoomLevelDelegate::ZoomLevelDelegate(PrefService* pref_service,
                                     const base::FilePath& partition_path)
    : pref_service_(pref_service) {
  DCHECK(pref_service_);
  partition_key_ = GetHash(partition_path);
}

ZoomLevelDelegate::~ZoomLevelDelegate() = default;

void ZoomLevelDelegate::SetDefaultZoomLevelPref(double level) {
  if (blink::PageZoomValuesEqual(level, host_zoom_map_->GetDefaultZoomLevel()))
    return;

  DictionaryPrefUpdate update(pref_service_, kPartitionDefaultZoomLevel);
  update->SetDouble(partition_key_, level);
  host_zoom_map_->SetDefaultZoomLevel(level);
}

double ZoomLevelDelegate::GetDefaultZoomLevelPref() const {
  double default_zoom_level = 0.0;

  const base::DictionaryValue* default_zoom_level_dictionary =
      pref_service_->GetDictionary(kPartitionDefaultZoomLevel);
  // 如果以前没有设置默认值，则返回的默认值为。
  // 用于初始化此函数中的DEFAULT_ZOOM_LEVEL的值。
  default_zoom_level_dictionary->GetDouble(partition_key_, &default_zoom_level);
  return default_zoom_level;
}

void ZoomLevelDelegate::OnZoomLevelChanged(
    const content::HostZoomMap::ZoomLevelChange& change) {
  if (change.mode != content::HostZoomMap::ZOOM_CHANGED_FOR_HOST)
    return;

  double level = change.zoom_level;
  DictionaryPrefUpdate update(pref_service_, kPartitionPerHostZoomLevels);
  base::DictionaryValue* host_zoom_dictionaries = update.Get();
  DCHECK(host_zoom_dictionaries);

  bool modification_is_removal =
      blink::PageZoomValuesEqual(level, host_zoom_map_->GetDefaultZoomLevel());

  base::DictionaryValue* host_zoom_dictionary = nullptr;
  if (!host_zoom_dictionaries->GetDictionary(partition_key_,
                                             &host_zoom_dictionary)) {
    host_zoom_dictionary = host_zoom_dictionaries->SetDictionary(
        partition_key_, std::make_unique<base::DictionaryValue>());
  }

  if (modification_is_removal)
    host_zoom_dictionary->RemoveKey(change.host);
  else
    host_zoom_dictionary->SetKey(change.host, base::Value(level));
}

void ZoomLevelDelegate::ExtractPerHostZoomLevels(
    const base::DictionaryValue* host_zoom_dictionary) {
  std::vector<std::string> keys_to_remove;
  std::unique_ptr<base::DictionaryValue> host_zoom_dictionary_copy =
      host_zoom_dictionary->DeepCopyWithoutEmptyChildren();
  for (base::DictionaryValue::Iterator i(*host_zoom_dictionary_copy);
       !i.IsAtEnd(); i.Advance()) {
    const std::string& host(i.key());
    const absl::optional<double> zoom_level = i.value().GetIfDouble();

    // 过滤掉A)空主机，B)等于默认值的缩放级别；以及。
    // 记住它们，这样我们以后就可以从首选项中删除它们。
    // 类型B的值可以在默认缩放之前进一步存储。
    // 级别已设置为其当前值。在这两种情况下，SetZoomLevelForHost。
    // 将忽略类型B值，因此与HostZoomMap的值保持一致。
    // 内部状态，则还必须从首选项中删除这些值。
    if (host.empty() || !zoom_level ||
        blink::PageZoomValuesEqual(*zoom_level,
                                   host_zoom_map_->GetDefaultZoomLevel())) {
      keys_to_remove.push_back(host);
      continue;
    }

    host_zoom_map_->SetZoomLevelForHost(host, *zoom_level);
  }

  // 清理首选项以删除与默认缩放级别和/或。
  // 有一个空的主机。
  {
    DictionaryPrefUpdate update(pref_service_, kPartitionPerHostZoomLevels);
    base::DictionaryValue* host_zoom_dictionaries = update.Get();
    base::DictionaryValue* sanitized_host_zoom_dictionary = nullptr;
    host_zoom_dictionaries->GetDictionary(partition_key_,
                                          &sanitized_host_zoom_dictionary);
    for (const std::string& s : keys_to_remove)
      sanitized_host_zoom_dictionary->RemoveKey(s);
  }
}

void ZoomLevelDelegate::InitHostZoomMap(content::HostZoomMap* host_zoom_map) {
  // 此init函数只能调用一次。
  DCHECK(!host_zoom_map_);
  DCHECK(host_zoom_map);
  host_zoom_map_ = host_zoom_map;

  // 初始化默认缩放级别。
  host_zoom_map_->SetDefaultZoomLevel(GetDefaultZoomLevelPref());

  // 使用持久化的每个主机的缩放级别初始化HostZoomMap。
  // 缩放级别首选项值。
  const base::DictionaryValue* host_zoom_dictionaries =
      pref_service_->GetDictionary(kPartitionPerHostZoomLevels);
  const base::DictionaryValue* host_zoom_dictionary = nullptr;
  if (host_zoom_dictionaries->GetDictionary(partition_key_,
                                            &host_zoom_dictionary)) {
    // 由于我们在设置下面的Zoom_Subscription_之前调用它，因此我们。
    // 无需担心HOST_ZOOM_DICTIONARY会受到间接影响。
    // 通过调用HostZoomMap：：SetZoomLevelForHost()。
    ExtractPerHostZoomLevels(host_zoom_dictionary);
  }
  zoom_subscription_ =
      host_zoom_map_->AddZoomLevelChangedCallback(base::BindRepeating(
          &ZoomLevelDelegate::OnZoomLevelChanged, base::Unretained(this)));
}

}  // 命名空间电子
