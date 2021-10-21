// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/net/network_context_service_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "shell/browser/net/network_context_service.h"

namespace electron {

NetworkContextService* NetworkContextServiceFactory::GetForContext(
    content::BrowserContext* browser_context) {
  return static_cast<NetworkContextService*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

NetworkContextServiceFactory* NetworkContextServiceFactory::GetInstance() {
  return base::Singleton<NetworkContextServiceFactory>::get();
}

NetworkContextServiceFactory::NetworkContextServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ElectronNetworkContextService",
          BrowserContextDependencyManager::GetInstance()) {}

NetworkContextServiceFactory::~NetworkContextServiceFactory() = default;

KeyedService* NetworkContextServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new NetworkContextService(
      static_cast<ElectronBrowserContext*>(context));
}

content::BrowserContext* NetworkContextServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // 为临时会话创建单独的服务。
  return context;
}

}  // 命名空间电子
