// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NET_PROXYING_URL_LOADER_FACTORY_H_
#define SHELL_BROWSER_NET_PROXYING_URL_LOADER_FACTORY_H_

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/completion_once_callback.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "shell/browser/net/electron_url_loader_factory.h"
#include "shell/browser/net/web_request_api_interface.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace electron {

// 启用NetworkService后，此类负责执行以下任务：
// 1.处理截获的协议；
// 2.实现WebRequest模块；
// 
// 对于任务#2，代码引用自。
// Extensions：：WebRequestProxyingURLLoaderFactory类。
class ProxyingURLLoaderFactory
    : public network::mojom::URLLoaderFactory,
      public network::mojom::TrustedURLLoaderHeaderClient {
 public:
  class InProgressRequest : public network::mojom::URLLoader,
                            public network::mojom::URLLoaderClient,
                            public network::mojom::TrustedHeaderClient {
   public:
    // 对于通常的请求。
    InProgressRequest(
        ProxyingURLLoaderFactory* factory,
        uint64_t web_request_id,
        int32_t view_routing_id,
        int32_t frame_routing_id,
        int32_t network_service_request_id,
        uint32_t options,
        const network::ResourceRequest& request,
        const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
        mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
        mojo::PendingRemote<network::mojom::URLLoaderClient> client);
    // 用于CORS飞行前检查。
    InProgressRequest(ProxyingURLLoaderFactory* factory,
                      uint64_t request_id,
                      int32_t frame_routing_id,
                      const network::ResourceRequest& request);
    ~InProgressRequest() override;

    void Restart();

    // Network：：mojom：：URLLoader：
    void FollowRedirect(
        const std::vector<std::string>& removed_headers,
        const net::HttpRequestHeaders& modified_headers,
        const net::HttpRequestHeaders& modified_cors_exempt_headers,
        const absl::optional<GURL>& new_url) override;
    void SetPriority(net::RequestPriority priority,
                     int32_t intra_priority_value) override;
    void PauseReadingBodyFromNet() override;
    void ResumeReadingBodyFromNet() override;

    // Network：：mojom：：URLLoaderClient：
    void OnReceiveEarlyHints(
        network::mojom::EarlyHintsPtr early_hints) override;
    void OnReceiveResponse(network::mojom::URLResponseHeadPtr head) override;
    void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                           network::mojom::URLResponseHeadPtr head) override;
    void OnUploadProgress(int64_t current_position,
                          int64_t total_size,
                          OnUploadProgressCallback callback) override;
    void OnReceiveCachedMetadata(mojo_base::BigBuffer data) override;
    void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
    void OnStartLoadingResponseBody(
        mojo::ScopedDataPipeConsumerHandle body) override;
    void OnComplete(const network::URLLoaderCompletionStatus& status) override;

    void OnLoaderCreated(
        mojo::PendingReceiver<network::mojom::TrustedHeaderClient> receiver);

    // Network：：mojom：：trudHeaderClient：
    void OnBeforeSendHeaders(const net::HttpRequestHeaders& headers,
                             OnBeforeSendHeadersCallback callback) override;
    void OnHeadersReceived(const std::string& headers,
                           const net::IPEndPoint& endpoint,
                           OnHeadersReceivedCallback callback) override;

   private:
    // 这两种方法结合在一起形成了Restart()的实现。
    void UpdateRequestInfo();
    void RestartInternal();

    void ContinueToBeforeSendHeaders(int error_code);
    void ContinueToSendHeaders(const std::set<std::string>& removed_headers,
                               const std::set<std::string>& set_headers,
                               int error_code);
    void ContinueToStartRequest(int error_code);
    void ContinueToHandleOverrideHeaders(int error_code);
    void ContinueToResponseStarted(int error_code);
    void ContinueToBeforeRedirect(const net::RedirectInfo& redirect_info,
                                  int error_code);
    void HandleResponseOrRedirectHeaders(
        net::CompletionOnceCallback continuation);
    void OnRequestError(const network::URLLoaderCompletionStatus& status);
    void HandleBeforeRequestRedirect();

    ProxyingURLLoaderFactory* const factory_;
    network::ResourceRequest request_;
    const absl::optional<url::Origin> original_initiator_;
    const uint64_t request_id_ = 0;
    const int32_t network_service_request_id_ = 0;
    const int32_t view_routing_id_ = MSG_ROUTING_NONE;
    const int32_t frame_routing_id_ = MSG_ROUTING_NONE;
    const uint32_t options_ = 0;
    const net::MutableNetworkTrafficAnnotationTag traffic_annotation_;
    mojo::Receiver<network::mojom::URLLoader> proxied_loader_receiver_;
    mojo::Remote<network::mojom::URLLoaderClient> target_client_;

    absl::optional<extensions::WebRequestInfo> info_;

    mojo::Receiver<network::mojom::URLLoaderClient> proxied_client_receiver_{
        this};
    network::mojom::URLLoaderPtr target_loader_;

    network::mojom::URLResponseHeadPtr current_response_;
    scoped_refptr<net::HttpResponseHeaders> override_headers_;
    GURL redirect_url_;

    const bool for_cors_preflight_ = false;

    // 如果|HAS_ANY_EXTRA_HEADERS_LISTENERS_|设置为TRUE，则请求将为。
    // 使用network：：mojom：：kURLLoadOptionUseHeaderClient选项发送，并且。
    // 我们预计事件将通过。
    // 工厂上的Network：：mojom：：trudURLLoaderHeaderClient绑定。这。
    // 仅当存在需要查看或修改的监听器时才设置为true。
    // 在网络进程中设置的标头。
    bool has_any_extra_headers_listeners_ = false;
    bool current_request_uses_header_client_ = false;
    OnBeforeSendHeadersCallback on_before_send_headers_callback_;
    OnHeadersReceivedCallback on_headers_received_callback_;
    mojo::Receiver<network::mojom::TrustedHeaderClient> header_client_receiver_{
        this};

    // 如果|HAS_ANY_EXTRA_HEADERS_LISTENERS_|设置为FALSE且重定向为。
    // 在执行过程中，这会将参数存储到FollowRedirect，这些参数来自。
    // 客户。这样，我们就可以将其与任何其他更改结合在一起。
    // 在其回调中对标头进行了扩展。
    struct FollowRedirectParams {
      FollowRedirectParams();
      ~FollowRedirectParams();
      std::vector<std::string> removed_headers;
      net::HttpRequestHeaders modified_headers;
      net::HttpRequestHeaders modified_cors_exempt_headers;
      absl::optional<GURL> new_url;

      DISALLOW_COPY_AND_ASSIGN(FollowRedirectParams);
    };
    std::unique_ptr<FollowRedirectParams> pending_follow_redirect_params_;

    base::WeakPtrFactory<InProgressRequest> weak_factory_{this};

    DISALLOW_COPY_AND_ASSIGN(InProgressRequest);
  };

  ProxyingURLLoaderFactory(
      WebRequestAPI* web_request_api,
      const HandlersMap& intercepted_handlers,
      int render_process_id,
      int frame_routing_id,
      int view_routing_id,
      uint64_t* request_id_generator,
      std::unique_ptr<extensions::ExtensionNavigationUIData> navigation_ui_data,
      absl::optional<int64_t> navigation_id,
      network::mojom::URLLoaderFactoryRequest loader_request,
      mojo::PendingRemote<network::mojom::URLLoaderFactory>
          target_factory_remote,
      mojo::PendingReceiver<network::mojom::TrustedURLLoaderHeaderClient>
          header_client_receiver,
      content::ContentBrowserClient::URLLoaderFactoryType loader_factory_type);

  ~ProxyingURLLoaderFactory() override;

  // Network：：mojom：：URLLoaderFactory：
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override;
  void Clone(mojo::PendingReceiver<network::mojom::URLLoaderFactory>
                 loader_receiver) override;

  // Network：：mojom：：trudURLLoaderHeaderClient：
  void OnLoaderCreated(
      int32_t request_id,
      mojo::PendingReceiver<network::mojom::TrustedHeaderClient> receiver)
      override;
  void OnLoaderForCorsPreflightCreated(
      const network::ResourceRequest& request,
      mojo::PendingReceiver<network::mojom::TrustedHeaderClient> receiver)
      override;

  WebRequestAPI* web_request_api() { return web_request_api_; }

  bool IsForServiceWorkerScript() const;

 private:
  void OnTargetFactoryError();
  void OnProxyBindingError();
  void RemoveRequest(int32_t network_service_request_id, uint64_t request_id);
  void MaybeDeleteThis();

  bool ShouldIgnoreConnectionsLimit(const network::ResourceRequest& request);

  // 从API：：WebRequest传入。
  WebRequestAPI* web_request_api_;

  // 这是从API：：Protocol传入的。
  // 
  // 协议实例存在于BrowserContext的整个生命周期中，
  // 它保证覆盖URLLoaderFactory的整个生命周期，因此。
  // 保证引用是有效的。
  // 
  // 这样，我们就可以避免在该文件中使用API命名空间中的代码。
  const HandlersMap& intercepted_handlers_;

  const int render_process_id_;
  const int frame_routing_id_;
  const int view_routing_id_;
  uint64_t* request_id_generator_;  // 由ElectronBrowserClient管理。
  std::unique_ptr<extensions::ExtensionNavigationUIData> navigation_ui_data_;
  absl::optional<int64_t> navigation_id_;
  mojo::ReceiverSet<network::mojom::URLLoaderFactory> proxy_receivers_;
  mojo::Remote<network::mojom::URLLoaderFactory> target_factory_;
  mojo::Receiver<network::mojom::TrustedURLLoaderHeaderClient>
      url_loader_header_client_receiver_{this};
  const content::ContentBrowserClient::URLLoaderFactoryType
      loader_factory_type_;

  // 从我们内部生成的请求ID映射到。
  // InProgressRequest实例。
  std::map<uint64_t, std::unique_ptr<InProgressRequest>> requests_;

  // 从网络堆栈的请求ID概念到我们自己的映射。
  // 内部为同一请求生成的请求ID。
  std::map<int32_t, uint64_t> network_request_id_to_web_request_id_;

  std::vector<std::string> ignore_connections_limit_domains_;

  DISALLOW_COPY_AND_ASSIGN(ProxyingURLLoaderFactory);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NET_PROXYING_URL_LOADER_FACTORY_H_
