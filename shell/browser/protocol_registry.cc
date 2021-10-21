// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/protocol_registry.h"

#include "base/stl_util.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/net/asar/asar_url_loader_factory.h"

namespace electron {

// 静电。
ProtocolRegistry* ProtocolRegistry::FromBrowserContext(
    content::BrowserContext* context) {
  return static_cast<ElectronBrowserContext*>(context)->protocol_registry();
}

ProtocolRegistry::ProtocolRegistry() = default;

ProtocolRegistry::~ProtocolRegistry() = default;

void ProtocolRegistry::RegisterURLLoaderFactories(
    content::ContentBrowserClient::NonNetworkURLLoaderFactoryMap* factories,
    bool allow_file_access) {
  auto file_factory = factories->find(url::kFileScheme);
  if (file_factory != factories->end()) {
    // 如果Chromium已经允许文件访问，则将url工厂替换为。
    // 也在加载asar文件。
    file_factory->second = AsarURLLoaderFactory::Create();
  } else if (allow_file_access) {
    // 否则，仅在显式允许的情况下才允许文件访问。
    // 
    // 请注意，Chromium可能会调用|emplace|来创建默认文件工厂。
    // 在此调用之后，它不会覆盖我们的asar工厂，但如果asar支持。
    // 以后休息时，请检查Chromium是否更改了通话。
    factories->emplace(url::kFileScheme, AsarURLLoaderFactory::Create());
  }

  for (const auto& it : handlers_) {
    factories->emplace(it.first, ElectronURLLoaderFactory::Create(
                                     it.second.first, it.second.second));
  }
}

bool ProtocolRegistry::RegisterProtocol(ProtocolType type,
                                        const std::string& scheme,
                                        const ProtocolHandler& handler) {
  return base::TryEmplace(handlers_, scheme, type, handler).second;
}

bool ProtocolRegistry::UnregisterProtocol(const std::string& scheme) {
  return handlers_.erase(scheme) != 0;
}

bool ProtocolRegistry::IsProtocolRegistered(const std::string& scheme) {
  return base::Contains(handlers_, scheme);
}

bool ProtocolRegistry::InterceptProtocol(ProtocolType type,
                                         const std::string& scheme,
                                         const ProtocolHandler& handler) {
  return base::TryEmplace(intercept_handlers_, scheme, type, handler).second;
}

bool ProtocolRegistry::UninterceptProtocol(const std::string& scheme) {
  return intercept_handlers_.erase(scheme) != 0;
}

bool ProtocolRegistry::IsProtocolIntercepted(const std::string& scheme) {
  return base::Contains(intercept_handlers_, scheme);
}

}  // 命名空间电子
