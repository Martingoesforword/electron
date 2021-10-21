// 版权所有2020年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NET_PROXYING_WEBSOCKET_H_
#define SHELL_BROWSER_NET_PROXYING_WEBSOCKET_H_

#include <set>
#include <string>
#include <vector>

#include "content/public/browser/content_browser_client.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "shell/browser/net/web_request_api_interface.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace electron {

// ProxyingWebSocket代理WebSocket连接并分派。
// WebRequest API事件。
// 
// 该代码是从。
// Extensions：：WebRequestProxyingWebSocket类。
class ProxyingWebSocket : public network::mojom::WebSocketHandshakeClient,
                          public network::mojom::WebSocketAuthenticationHandler,
                          public network::mojom::TrustedHeaderClient {
 public:
  using WebSocketFactory = content::ContentBrowserClient::WebSocketFactory;

  // AuthRequiredResponse指示如何处理OnAuthRequired调用。
  enum class AuthRequiredResponse {
    // 未提供凭据。
    kNoAction,
    // AuthCredentials是用用户名和密码填写的，应该是。
    // 用于响应提供的身份验证质询。
    kSetAuth,
    // 该请求应被取消。
    kCancelAuth,
    // 行动将以异步方式决定。|回调|将被调用。
    // 做出决定时，其他AuthRequiredResponse中的一个。
    // 值将以与上述相同的语义传入。
    kIoPending,
  };

  ProxyingWebSocket(
      WebRequestAPI* web_request_api,
      WebSocketFactory factory,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client,
      bool has_extra_headers,
      int process_id,
      int render_frame_id,
      content::BrowserContext* browser_context,
      uint64_t* request_id_generator);
  ~ProxyingWebSocket() override;

  void Start();

  // Network：：mojom：：WebSocketHandshakeClient方法：
  void OnOpeningHandshakeStarted(
      network::mojom::WebSocketHandshakeRequestPtr request) override;
  void OnFailure(const std::string& message,
                 int32_t net_error,
                 int32_t response_code) override;
  void OnConnectionEstablished(
      mojo::PendingRemote<network::mojom::WebSocket> websocket,
      mojo::PendingReceiver<network::mojom::WebSocketClient> client_receiver,
      network::mojom::WebSocketHandshakeResponsePtr response,
      mojo::ScopedDataPipeConsumerHandle readable,
      mojo::ScopedDataPipeProducerHandle writable) override;

  // Network：：mojom：：WebSocketAuthenticationHandler方法：
  void OnAuthRequired(const net::AuthChallengeInfo& auth_info,
                      const scoped_refptr<net::HttpResponseHeaders>& headers,
                      const net::IPEndPoint& remote_endpoint,
                      OnAuthRequiredCallback callback) override;

  // Network：：mojom：：trudHeaderClient方法：
  void OnBeforeSendHeaders(const net::HttpRequestHeaders& headers,
                           OnBeforeSendHeadersCallback callback) override;
  void OnHeadersReceived(const std::string& headers,
                         const net::IPEndPoint& endpoint,
                         OnHeadersReceivedCallback callback) override;

  static void StartProxying(
      WebRequestAPI* web_request_api,
      WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client,
      bool has_extra_headers,
      int process_id,
      int render_frame_id,
      const url::Origin& origin,
      content::BrowserContext* browser_context,
      uint64_t* request_id_generator);

  WebRequestAPI* web_request_api() { return web_request_api_; }

 private:
  void OnBeforeRequestComplete(int error_code);
  void OnBeforeSendHeadersComplete(const std::set<std::string>& removed_headers,
                                   const std::set<std::string>& set_headers,
                                   int error_code);
  void ContinueToStartRequest(int error_code);
  void OnHeadersReceivedComplete(int error_code);
  void ContinueToHeadersReceived();
  void OnAuthRequiredComplete(AuthRequiredResponse rv);
  void OnHeadersReceivedCompleteForAuth(const net::AuthChallengeInfo& auth_info,
                                        int rv);
  void ContinueToCompleted();

  void PauseIncomingMethodCallProcessing();
  void ResumeIncomingMethodCallProcessing();
  void OnError(int error_code);
  // 这用于检测MOJO与网络连接的错误。
  // 服务。
  void OnMojoConnectionErrorWithCustomReason(uint32_t custom_reason,
                                             const std::string& description);
  // 用于检测与原始客户端的MOJO连接错误。
  // (即渲染器)。
  void OnMojoConnectionError();

  // 从API：：WebRequest传入。
  WebRequestAPI* web_request_api_;

  // 保存为馈送API：：WebRequest。
  network::ResourceRequest request_;

  WebSocketFactory factory_;
  mojo::Remote<network::mojom::WebSocketHandshakeClient>
      forwarding_handshake_client_;
  mojo::Receiver<network::mojom::WebSocketHandshakeClient>
      receiver_as_handshake_client_{this};
  mojo::Receiver<network::mojom::WebSocketAuthenticationHandler>
      receiver_as_auth_handler_{this};
  mojo::Receiver<network::mojom::TrustedHeaderClient>
      receiver_as_header_client_{this};

  net::HttpRequestHeaders request_headers_;
  network::mojom::URLResponseHeadPtr response_;
  net::AuthCredentials auth_credentials_;
  OnAuthRequiredCallback auth_required_callback_;
  scoped_refptr<net::HttpResponseHeaders> override_headers_;
  std::vector<network::mojom::HttpHeaderPtr> additional_headers_;

  OnBeforeSendHeadersCallback on_before_send_headers_callback_;
  OnHeadersReceivedCallback on_headers_received_callback_;

  GURL redirect_url_;
  bool is_done_ = false;
  bool has_extra_headers_;
  mojo::PendingRemote<network::mojom::WebSocket> websocket_;
  mojo::PendingReceiver<network::mojom::WebSocketClient> client_receiver_;
  network::mojom::WebSocketHandshakeResponsePtr handshake_response_ = nullptr;
  mojo::ScopedDataPipeConsumerHandle readable_;
  mojo::ScopedDataPipeProducerHandle writable_;

  extensions::WebRequestInfo info_;

  base::WeakPtrFactory<ProxyingWebSocket> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ProxyingWebSocket);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Net_Proxying_WebSocket_H_
