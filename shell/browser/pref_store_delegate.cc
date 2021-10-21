// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/pref_store_delegate.h"

#include <utility>

#include "components/prefs/persistent_pref_store.h"
#include "components/prefs/pref_store.h"
#include "components/prefs/value_map_pref_store.h"
#include "shell/browser/electron_browser_context.h"

namespace electron {

PrefStoreDelegate::PrefStoreDelegate(
    base::WeakPtr<ElectronBrowserContext> browser_context)
    : browser_context_(std::move(browser_context)) {}

PrefStoreDelegate::~PrefStoreDelegate() {
  if (browser_context_)
    browser_context_->set_in_memory_pref_store(nullptr);
}

void PrefStoreDelegate::UpdateCommandLinePrefStore(
    PrefStore* command_line_prefs) {
  if (browser_context_)
    browser_context_->set_in_memory_pref_store(
        static_cast<ValueMapPrefStore*>(command_line_prefs));
}

}  // 命名空间电子
