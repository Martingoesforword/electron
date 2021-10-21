// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_H_
#define SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_H_

#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace asar {

void CreateAsarURLLoader(
    const network::ResourceRequest& request,
    network::mojom::URLLoaderRequest loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    scoped_refptr<net::HttpResponseHeaders> extra_response_headers);

}  // 命名空间asar。

#endif  // Shell_Browser_Net_ASAR_ASAR_URL_LOADER_H_
