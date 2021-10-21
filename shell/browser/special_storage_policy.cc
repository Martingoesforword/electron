// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/special_storage_policy.h"

#include "base/bind.h"
#include "base/callback.h"

namespace electron {

SpecialStoragePolicy::SpecialStoragePolicy() = default;

SpecialStoragePolicy::~SpecialStoragePolicy() = default;

bool SpecialStoragePolicy::IsStorageProtected(const GURL& origin) {
  return true;
}

bool SpecialStoragePolicy::IsStorageUnlimited(const GURL& origin) {
  return true;
}

bool SpecialStoragePolicy::IsStorageDurable(const GURL& origin) {
  return true;
}

bool SpecialStoragePolicy::HasIsolatedStorage(const GURL& origin) {
  return false;
}

bool SpecialStoragePolicy::IsStorageSessionOnly(const GURL& origin) {
  return false;
}

bool SpecialStoragePolicy::HasSessionOnlyOrigins() {
  return false;
}

network::DeleteCookiePredicate
SpecialStoragePolicy::CreateDeleteCookieOnExitPredicate() {
  return network::DeleteCookiePredicate();
}

}  // 命名空间电子
