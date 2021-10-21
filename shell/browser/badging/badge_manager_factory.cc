// 版权所有2018年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/badging/badge_manager_factory.h"

#include <memory>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "shell/browser/badging/badge_manager.h"

namespace badging {

// 静电。
BadgeManager* BadgeManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<badging::BadgeManager*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// 静电。
BadgeManagerFactory* BadgeManagerFactory::GetInstance() {
  return base::Singleton<BadgeManagerFactory>::get();
}

BadgeManagerFactory::BadgeManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "BadgeManager",
          BrowserContextDependencyManager::GetInstance()) {}

BadgeManagerFactory::~BadgeManagerFactory() = default;

KeyedService* BadgeManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BadgeManager();
}

}  // 命名空间标记
