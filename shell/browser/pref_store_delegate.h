// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_PREF_STORE_DELEGATE_H_
#define SHELL_BROWSER_PREF_STORE_DELEGATE_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/prefs/pref_value_store.h"

class PersistentPrefStore;
class PrefNotifier;
class PrefRegistry;
class PrefStore;

namespace electron {

class ElectronBrowserContext;

// 检索内存中首选存储的句柄，该存储获取。
// 已使用首选项服务进行初始化。
class PrefStoreDelegate : public PrefValueStore::Delegate {
 public:
  explicit PrefStoreDelegate(
      base::WeakPtr<ElectronBrowserContext> browser_context);
  ~PrefStoreDelegate() override;

  void Init(PrefStore* managed_prefs,
            PrefStore* supervised_user_prefs,
            PrefStore* extension_prefs,
            PrefStore* command_line_prefs,
            PrefStore* user_prefs,
            PrefStore* recommended_prefs,
            PrefStore* default_prefs,
            PrefNotifier* pref_notifier) override {}

  void InitIncognitoUserPrefs(
      scoped_refptr<PersistentPrefStore> incognito_user_prefs_overlay,
      scoped_refptr<PersistentPrefStore> incognito_user_prefs_underlay,
      const std::vector<const char*>& overlay_pref_names) override {}

  void InitPrefRegistry(PrefRegistry* pref_registry) override {}

  void UpdateCommandLinePrefStore(PrefStore* command_line_prefs) override;

 private:
  base::WeakPtr<ElectronBrowserContext> browser_context_;

  DISALLOW_COPY_AND_ASSIGN(PrefStoreDelegate);
};

}  // 命名空间电子。

#endif  // Shell_Browser_PREF_STORE_Delegate_H_
