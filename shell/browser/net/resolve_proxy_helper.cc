// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/net/resolve_proxy_helper.h"

#include <utility>

#include "base/bind.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/proxy_resolution/proxy_info.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "shell/browser/electron_browser_context.h"

using content::BrowserThread;

namespace electron {

ResolveProxyHelper::ResolveProxyHelper(ElectronBrowserContext* browser_context)
    : browser_context_(browser_context) {}

ResolveProxyHelper::~ResolveProxyHelper() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!owned_self_);
  DCHECK(!receiver_.is_bound());
  // 如果ProxyService仍处于活动状态，则清除所有挂起的请求。
  pending_requests_.clear();
}

void ResolveProxyHelper::ResolveProxy(const GURL& url,
                                      ResolveProxyCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // 将挂起的请求排队。
  pending_requests_.emplace_back(url, std::move(callback));

  // 如果没有进行任何操作，请启动。
  if (!receiver_.is_bound()) {
    DCHECK_EQ(1u, pending_requests_.size());
    StartPendingRequest();
  }
}

void ResolveProxyHelper::StartPendingRequest() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!receiver_.is_bound());
  DCHECK(!pending_requests_.empty());

  // 启动请求。
  mojo::PendingRemote<network::mojom::ProxyLookupClient> proxy_lookup_client =
      receiver_.BindNewPipeAndPassRemote();
  receiver_.set_disconnect_handler(
      base::BindOnce(&ResolveProxyHelper::OnProxyLookupComplete,
                     base::Unretained(this), net::ERR_ABORTED, absl::nullopt));
  browser_context_->GetDefaultStoragePartition()
      ->GetNetworkContext()
      ->LookUpProxyForURL(pending_requests_.front().url,
                          net::NetworkIsolationKey(),
                          std::move(proxy_lookup_client));
}

void ResolveProxyHelper::OnProxyLookupComplete(
    int32_t net_error,
    const absl::optional<net::ProxyInfo>& proxy_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!pending_requests_.empty());

  receiver_.reset();

  // 清除当前(已完成)请求。
  PendingRequest completed_request = std::move(pending_requests_.front());
  pending_requests_.pop_front();

  std::string proxy;
  if (proxy_info)
    proxy = proxy_info->ToPacString();

  if (!completed_request.callback.is_null())
    std::move(completed_request.callback).Run(proxy);

  // 开始下一个请求。
  if (!pending_requests_.empty())
    StartPendingRequest();
}

ResolveProxyHelper::PendingRequest::PendingRequest(
    const GURL& url,
    ResolveProxyCallback callback)
    : url(url), callback(std::move(callback)) {}

ResolveProxyHelper::PendingRequest::PendingRequest(
    ResolveProxyHelper::PendingRequest&& pending_request) = default;

ResolveProxyHelper::PendingRequest::~PendingRequest() noexcept = default;

ResolveProxyHelper::PendingRequest&
ResolveProxyHelper::PendingRequest::operator=(
    ResolveProxyHelper::PendingRequest&& pending_request) noexcept = default;

}  // 命名空间电子
