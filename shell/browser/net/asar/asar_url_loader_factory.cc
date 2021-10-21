// 版权所有(C)2021 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/net/asar/asar_url_loader_factory.h"

#include <utility>

#include "shell/browser/net/asar/asar_url_loader.h"

namespace electron {

// 静电。
mojo::PendingRemote<network::mojom::URLLoaderFactory>
AsarURLLoaderFactory::Create() {
  mojo::PendingRemote<network::mojom::URLLoaderFactory> pending_remote;

  // AsarURLLoaderFactory将在没有更多。
  // 接收器-请参见SelfDeletingURLLoaderFactory：：OnDisconnect方法。
  new AsarURLLoaderFactory(pending_remote.InitWithNewPipeAndPassReceiver());

  return pending_remote;
}

AsarURLLoaderFactory::AsarURLLoaderFactory(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver)
    : network::SelfDeletingURLLoaderFactory(std::move(factory_receiver)) {}
AsarURLLoaderFactory::~AsarURLLoaderFactory() = default;

void AsarURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  asar::CreateAsarURLLoader(request, std::move(loader), std::move(client),
                            new net::HttpResponseHeaders(""));
}

}  // 命名空间电子
