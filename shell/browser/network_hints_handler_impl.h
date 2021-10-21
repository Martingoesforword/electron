// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NETWORK_HINTS_HANDLER_IMPL_H_
#define SHELL_BROWSER_NETWORK_HINTS_HANDLER_IMPL_H_

#include "components/network_hints/browser/simple_network_hints_handler_impl.h"

namespace content {
class RenderFrameHost;
class BrowserContext;
}  // 命名空间内容。

class NetworkHintsHandlerImpl
    : public network_hints::SimpleNetworkHintsHandlerImpl {
 public:
  ~NetworkHintsHandlerImpl() override;

  static void Create(
      content::RenderFrameHost* frame_host,
      mojo::PendingReceiver<network_hints::mojom::NetworkHintsHandler>
          receiver);

  // Network_Hints：：mojom：：NetworkHintsHandler：
  void Preconnect(const GURL& url, bool allow_credentials) override;

 private:
  explicit NetworkHintsHandlerImpl(content::RenderFrameHost*);

  content::BrowserContext* browser_context_ = nullptr;
};

#endif  // Shell_Browser_NETWORK_HINTS_HANDLER_IMPLL_H_
