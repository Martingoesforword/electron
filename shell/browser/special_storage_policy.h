// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_SPECIAL_STORAGE_POLICY_H_
#define SHELL_BROWSER_SPECIAL_STORAGE_POLICY_H_

#include "storage/browser/quota/special_storage_policy.h"

namespace electron {

class SpecialStoragePolicy : public storage::SpecialStoragePolicy {
 public:
  SpecialStoragePolicy();

  // 存储：：SpecialStoragePolicy实现。
  bool IsStorageProtected(const GURL& origin) override;
  bool IsStorageUnlimited(const GURL& origin) override;
  bool IsStorageDurable(const GURL& origin) override;
  bool HasIsolatedStorage(const GURL& origin) override;
  bool IsStorageSessionOnly(const GURL& origin) override;
  bool HasSessionOnlyOrigins() override;
  network::DeleteCookiePredicate CreateDeleteCookieOnExitPredicate() override;

 protected:
  ~SpecialStoragePolicy() override;
};

}  // 命名空间电子。

#endif  // Shell_Browser_Special_STORAGE_POLICY_H_
