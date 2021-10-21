// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NET_SYSTEM_NETWORK_CONTEXT_MANAGER_H_
#define SHELL_BROWSER_NET_SYSTEM_NETWORK_CONTEXT_MANAGER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/net/proxy_config_monitor.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace electron {
network::mojom::HttpAuthDynamicParamsPtr CreateHttpAuthDynamicParams();
}

// 负责创建和管理对系统NetworkContext的访问。
// 驻留在UI线程上。此用户拥有的NetworkContext用于请求。
// 不与会话相关联。它不在磁盘上存储数据，并且没有HTTP。
// 缓存，但它确实有短暂的cookie和频道ID存储。
// 
// 此类还负责配置全局网络服务状态。
// 
// 系统NetworkContext将与URLRequestContext共享URLRequestContext。
// IOThread的SystemURLRequestContext，并成为IOThread的网络服务的一部分。
// (如果网络服务被禁用)或作为独立的NetworkContext。
// 使用实际的网络服务。
class SystemNetworkContextManager {
 public:
  ~SystemNetworkContextManager();

  // 创建SystemNetworkContextManager的全局实例。如果一个。
  // 实例已存在，这将导致DCHECK失败。
  static SystemNetworkContextManager* CreateInstance(PrefService* pref_service);

  // 获取全局SystemNetworkContextManager实例。
  static SystemNetworkContextManager* GetInstance();

  // 销毁全局SystemNetworkContextManager实例。
  static void DeleteInstance();

  // 配置用于配置网络环境的默认参数集。
  void ConfigureDefaultNetworkContextParams(
      network::mojom::NetworkContextParams* network_context_params);

  // 与ConfigureDefaultNetworkContextParams()相同，但返回一个新的。
  // 分配的network：：mojom：：NetworkContextParams。
  // CertVerifierCreationParams已放入NetworkContextParams中。
  network::mojom::NetworkContextParamsPtr CreateDefaultNetworkContextParams();

  // 返回系统NetworkContext。只能在Setup()之后调用。会吗？
  // 首次启动时可能需要的任何网络服务初始化。
  // 打过电话。
  network::mojom::NetworkContext* GetContext();

  // 返回SystemNetworkContextManager拥有的URLLoaderFactory，该。
  // 以SystemNetworkContext为后盾。允许共享URLLoaderFactory。
  // 我更喜欢这样，而不是创建一个新的。对返回的值调用Clone()。
  // 此方法可获取可在其他线程上使用的URLLoaderFactory。
  network::mojom::URLLoaderFactory* GetURLLoaderFactory();

  // 返回SystemNetworkContextManager拥有的SharedURLLoaderFactory。
  // 这是由SystemNetworkContext支持的。
  scoped_refptr<network::SharedURLLoaderFactory> GetSharedURLLoaderFactory();

  // 当Content创建NetworkService时调用。创建。
  // 如果启用了网络服务，则返回SystemNetworkContext。
  void OnNetworkServiceCreated(network::mojom::NetworkService* network_service);

 private:
  class URLLoaderFactoryForSystem;

  explicit SystemNetworkContextManager(PrefService* pref_service);

  // 为NetworkContext创建参数。只能调用一次，因为。
  // 它初始化一些类成员。
  network::mojom::NetworkContextParamsPtr CreateNetworkContextParams();

  ProxyConfigMonitor proxy_config_monitor_;

  // 使用网络服务的NetworkContext(如果网络服务是。
  // 已启用。Nullptr，否则返回。
  mojo::Remote<network::mojom::NetworkContext> network_context_;

  // 由GetContext()返回的NetworkContext支持的URLLoaderFactory，因此。
  // 并不是所有的消费者都需要建立自己的工厂。
  scoped_refptr<URLLoaderFactoryForSystem> shared_url_loader_factory_;
  mojo::Remote<network::mojom::URLLoaderFactory> url_loader_factory_;

  DISALLOW_COPY_AND_ASSIGN(SystemNetworkContextManager);
};

#endif  // SHELL_BROWSER_NET_SYSTEM_NETWORK_CONTEXT_MANAGER_H_
