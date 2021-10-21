// 版权所有(C)2021 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_FACTORY_H_
#define SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_FACTORY_H_

#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/self_deleting_url_loader_factory.h"

namespace electron {

// 支持访问file：//protocol中的asar档案。
class AsarURLLoaderFactory : public network::SelfDeletingURLLoaderFactory {
 public:
  static mojo::PendingRemote<network::mojom::URLLoaderFactory> Create();

 private:
  AsarURLLoaderFactory(
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver);
  ~AsarURLLoaderFactory() override;

  // Network：：mojom：：URLLoaderFactory：
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_FACTORY_H_
