// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NET_RESOLVE_PROXY_HELPER_H_
#define SHELL_BROWSER_NET_RESOLVE_PROXY_HELPER_H_

#include <deque>
#include <string>

#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/network/public/mojom/proxy_lookup_client.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace electron {

class ElectronBrowserContext;

class ResolveProxyHelper
    : public base::RefCountedThreadSafe<ResolveProxyHelper>,
      network::mojom::ProxyLookupClient {
 public:
  using ResolveProxyCallback = base::OnceCallback<void(std::string)>;

  explicit ResolveProxyHelper(ElectronBrowserContext* browser_context);

  void ResolveProxy(const GURL& url, ResolveProxyCallback callback);

 protected:
  ~ResolveProxyHelper() override;

 private:
  friend class base::RefCountedThreadSafe<ResolveProxyHelper>;
  // PendingRequest是正在进行或排队的解析请求。
  struct PendingRequest {
   public:
    PendingRequest(const GURL& url, ResolveProxyCallback callback);
    PendingRequest(PendingRequest&& pending_request) noexcept;
    ~PendingRequest();

    PendingRequest& operator=(PendingRequest&& pending_request) noexcept;

    GURL url;
    ResolveProxyCallback callback;

   private:
    DISALLOW_COPY_AND_ASSIGN(PendingRequest);
  };

  // 启动第一个挂起的请求。
  void StartPendingRequest();

  // Network：：mojom：：ProxyLookupClient实现。
  void OnProxyLookupComplete(
      int32_t net_error,
      const absl::optional<net::ProxyInfo>& proxy_info) override;

  // 自我参照。只要有出色的代理查询就可以了。
  scoped_refptr<ResolveProxyHelper> owned_self_;

  std::deque<PendingRequest> pending_requests_;
  // 当前正在进行的请求的接收方(如果有)。
  mojo::Receiver<network::mojom::ProxyLookupClient> receiver_{this};

  // 弱参照。
  ElectronBrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(ResolveProxyHelper);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Net_Resolve_Proxy_Helper_H_
