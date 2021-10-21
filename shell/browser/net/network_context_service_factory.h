// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NET_NETWORK_CONTEXT_SERVICE_FACTORY_H_
#define SHELL_BROWSER_NET_NETWORK_CONTEXT_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class KeyedService;

namespace content {
class BrowserContext;
}

namespace electron {

class NetworkContextService;

class NetworkContextServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  // 返回支持NetworkContext的NetworkContextService。
  // |BROWSER_CONTEXT|。
  static NetworkContextService* GetForContext(
      content::BrowserContext* browser_context);

  // 返回NetworkContextServiceFactory单例。
  static NetworkContextServiceFactory* GetInstance();

  NetworkContextServiceFactory(const NetworkContextServiceFactory&) = delete;
  NetworkContextServiceFactory& operator=(const NetworkContextServiceFactory&) =
      delete;

 private:
  friend struct base::DefaultSingletonTraits<NetworkContextServiceFactory>;

  NetworkContextServiceFactory();
  ~NetworkContextServiceFactory() override;

  // BrowserContextKeyedServiceFactory实现：
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NET_NETWORK_CONTEXT_SERVICE_FACTORY_H_
