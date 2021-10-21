// 版权所有2018年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_BADGING_BADGE_MANAGER_FACTORY_H_
#define SHELL_BROWSER_BADGING_BADGE_MANAGER_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace badging {

class BadgeManager;

// 提供对上下文特定BadgeManager的访问的Singleton。
class BadgeManagerFactory : public BrowserContextKeyedServiceFactory {
 public:
  // 获取指定上下文的BadgeManager。
  static BadgeManager* GetForBrowserContext(content::BrowserContext* context);

  // 返回BadgeManagerFactory单例。
  static BadgeManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BadgeManagerFactory>;

  BadgeManagerFactory();
  ~BadgeManagerFactory() override;

  // BrowserContextKeyedServiceFactory。
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BadgeManagerFactory);
};

}  // 命名空间标记。

#endif  // SHELL_BROWSER_BADGING_BADGE_MANAGER_FACTORY_H_
