// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/net/proxying_url_loader_factory.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/extension_navigation_ui_data.h"
#include "net/base/completion_repeating_callback.h"
#include "net/base/load_flags.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/http/http_util.h"
#include "net/url_request/redirect_info.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/mojom/early_hints.mojom.h"
#include "shell/browser/net/asar/asar_url_loader.h"
#include "shell/common/options_switches.h"
#include "url/origin.h"

namespace electron {

ProxyingURLLoaderFactory::InProgressRequest::FollowRedirectParams::
    FollowRedirectParams() = default;
ProxyingURLLoaderFactory::InProgressRequest::FollowRedirectParams::
    ~FollowRedirectParams() = default;

ProxyingURLLoaderFactory::InProgressRequest::InProgressRequest(
    ProxyingURLLoaderFactory* factory,
    uint64_t web_request_id,
    int32_t view_routing_id,
    int32_t frame_routing_id,
    int32_t network_service_request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client)
    : factory_(factory),
      request_(request),
      original_initiator_(request.request_initiator),
      request_id_(web_request_id),
      network_service_request_id_(network_service_request_id),
      view_routing_id_(view_routing_id),
      frame_routing_id_(frame_routing_id),
      options_(options),
      traffic_annotation_(traffic_annotation),
      proxied_loader_receiver_(this, std::move(loader_receiver)),
      target_client_(std::move(client)),
      current_response_(network::mojom::URLResponseHead::New()),
      // 请始终使用Extra Headers模式，以兼容旧的接口，除非。
      // 当|request_id_|为零时，Chromium和。
      // 仅当请求从Net模块启动时，才会在Electron中发生。
      has_any_extra_headers_listeners_(network_service_request_id != 0) {
  // 如果存在客户端错误，请清理请求。
  target_client_.set_disconnect_handler(base::BindOnce(
      &ProxyingURLLoaderFactory::InProgressRequest::OnRequestError,
      weak_factory_.GetWeakPtr(),
      network::URLLoaderCompletionStatus(net::ERR_ABORTED)));
  proxied_loader_receiver_.set_disconnect_handler(base::BindOnce(
      &ProxyingURLLoaderFactory::InProgressRequest::OnRequestError,
      weak_factory_.GetWeakPtr(),
      network::URLLoaderCompletionStatus(net::ERR_ABORTED)));
}

ProxyingURLLoaderFactory::InProgressRequest::InProgressRequest(
    ProxyingURLLoaderFactory* factory,
    uint64_t request_id,
    int32_t frame_routing_id,
    const network::ResourceRequest& request)
    : factory_(factory),
      request_(request),
      original_initiator_(request.request_initiator),
      request_id_(request_id),
      frame_routing_id_(frame_routing_id),
      proxied_loader_receiver_(this),
      for_cors_preflight_(true),
      has_any_extra_headers_listeners_(true) {}

ProxyingURLLoaderFactory::InProgressRequest::~InProgressRequest() {
  // 这对于确保没有未完成的阻塞请求继续存在非常重要。
  // 引用此对象拥有的状态。
  if (info_) {
    factory_->web_request_api()->OnRequestWillBeDestroyed(&info_.value());
  }
  if (on_before_send_headers_callback_) {
    std::move(on_before_send_headers_callback_)
        .Run(net::ERR_ABORTED, absl::nullopt);
  }
  if (on_headers_received_callback_) {
    std::move(on_headers_received_callback_)
        .Run(net::ERR_ABORTED, absl::nullopt, absl::nullopt);
  }
}

void ProxyingURLLoaderFactory::InProgressRequest::Restart() {
  UpdateRequestInfo();
  RestartInternal();
}

void ProxyingURLLoaderFactory::InProgressRequest::UpdateRequestInfo() {
  // 随时调用|Restart()|时派生新的WebRequestInfo值，因为。
  // |REQUEST_|中的详细信息可能已更改，例如，如果我们已被重定向。
  // |REQUEST_INITIATOR|重定向时可以修改，但我们会保留原来的。
  // 对于|启动器|(在事件中)。另请参阅。
  // Https://developer.chrome.com/extensions/webRequest#event-onBeforeRequest.。
  network::ResourceRequest request_for_info = request_;
  request_for_info.request_initiator = original_initiator_;
  info_.emplace(extensions::WebRequestInfoInitParams(
      request_id_, factory_->render_process_id_, frame_routing_id_,
      factory_->navigation_ui_data_ ? factory_->navigation_ui_data_->DeepCopy()
                                    : nullptr,
      view_routing_id_, request_for_info, false,
      !(options_ & network::mojom::kURLLoadOptionSynchronous),
      factory_->IsForServiceWorkerScript(), factory_->navigation_id_,
      ukm::kInvalidSourceIdObj));

  current_request_uses_header_client_ =
      factory_->url_loader_header_client_receiver_.is_bound() &&
      (for_cors_preflight_ || network_service_request_id_ != 0) &&
      has_any_extra_headers_listeners_;
}

void ProxyingURLLoaderFactory::InProgressRequest::RestartInternal() {
  DCHECK_EQ(info_->url, request_.url)
      << "UpdateRequestInfo must have been called first";

  // 如果将使用头客户端，我们将立即启动请求，并且。
  // OnBeforeSendHeaders和OnSendHeaders将在那里处理。否则，
  // 在请求开始之前发送这些事件。
  base::RepeatingCallback<void(int)> continuation;
  if (current_request_uses_header_client_) {
    continuation = base::BindRepeating(
        &InProgressRequest::ContinueToStartRequest, weak_factory_.GetWeakPtr());
  } else if (for_cors_preflight_) {
    // 在本例中，我们什么也不做，因为扩展应该什么也看不到。
    return;
  } else {
    continuation =
        base::BindRepeating(&InProgressRequest::ContinueToBeforeSendHeaders,
                            weak_factory_.GetWeakPtr());
  }
  redirect_url_ = GURL();
  int result = factory_->web_request_api()->OnBeforeRequest(
      &info_.value(), request_, continuation, &redirect_url_);
  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    // 请求已同步取消。发送错误通知。
    // 并终止该请求。
    network::URLLoaderCompletionStatus status(result);
    OnRequestError(status);
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    // 一个或多个监听器正在阻塞，因此请求必须暂停到。
    // 他们会做出回应。|Continuation|以上将被异步调用。
    // 继续或取消请求。
    // 
    // 我们在这里暂停接收器，以防止进一步的客户端消息处理。
    if (proxied_client_receiver_.is_bound())
      proxied_client_receiver_.Pause();

    // 暂停头客户端，因为我们希望等到OnBeforeRequest。
    // 在处理任何未来事件之前完成。
    if (header_client_receiver_.is_bound())
      header_client_receiver_.Pause();
    return;
  }
  DCHECK_EQ(net::OK, result);

  continuation.Run(net::OK);
}

void ProxyingURLLoaderFactory::InProgressRequest::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const absl::optional<GURL>& new_url) {
  if (new_url)
    request_.url = new_url.value();

  for (const std::string& header : removed_headers)
    request_.headers.RemoveHeader(header);
  request_.headers.MergeFrom(modified_headers);

  // 在选中|CURRENT_REQUEST_USES_HEADER_CLIENT_|之前调用此函数。
  // 计算出来的。
  UpdateRequestInfo();

  if (target_loader_.is_bound()) {
    // 如果使用HEADER_CLIENT_，那么我们现在必须将FollowRedirect调用为。
    // 这就是触发网络服务回调的原因。
    // OnBeforeSendHeaders()。否则，现在不要调用FollowRedirect。等待。
    // 要运行onBeforeSendHeaders回调，因为这些回调可能会修改请求。
    // 标头，如果是这样，我们将把这些修改传递给FollowRedirect。
    if (current_request_uses_header_client_) {
      target_loader_->FollowRedirect(removed_headers, modified_headers,
                                     modified_cors_exempt_headers, new_url);
    } else {
      auto params = std::make_unique<FollowRedirectParams>();
      params->removed_headers = removed_headers;
      params->modified_headers = modified_headers;
      params->modified_cors_exempt_headers = modified_cors_exempt_headers;
      params->new_url = new_url;
      pending_follow_redirect_params_ = std::move(params);
    }
  }

  RestartInternal();
}

void ProxyingURLLoaderFactory::InProgressRequest::SetPriority(
    net::RequestPriority priority,
    int32_t intra_priority_value) {
  if (target_loader_.is_bound())
    target_loader_->SetPriority(priority, intra_priority_value);
}

void ProxyingURLLoaderFactory::InProgressRequest::PauseReadingBodyFromNet() {
  if (target_loader_.is_bound())
    target_loader_->PauseReadingBodyFromNet();
}

void ProxyingURLLoaderFactory::InProgressRequest::ResumeReadingBodyFromNet() {
  if (target_loader_.is_bound())
    target_loader_->ResumeReadingBodyFromNet();
}

void ProxyingURLLoaderFactory::InProgressRequest::OnReceiveEarlyHints(
    network::mojom::EarlyHintsPtr early_hints) {
  target_client_->OnReceiveEarlyHints(std::move(early_hints));
}

void ProxyingURLLoaderFactory::InProgressRequest::OnReceiveResponse(
    network::mojom::URLResponseHeadPtr head) {
  if (current_request_uses_header_client_) {
    // 使用我们从OnHeadersReceived获得的标头，因为它将包含
    // Set-Cookie(如果存在)。
    auto saved_headers = current_response_->headers;
    current_response_ = std::move(head);
    current_response_->headers = saved_headers;
    ContinueToResponseStarted(net::OK);
  } else {
    current_response_ = std::move(head);
    HandleResponseOrRedirectHeaders(
        base::BindOnce(&InProgressRequest::ContinueToResponseStarted,
                       weak_factory_.GetWeakPtr()));
  }
}

void ProxyingURLLoaderFactory::InProgressRequest::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    network::mojom::URLResponseHeadPtr head) {
  // 注意：在Electron中，我们不选中IsRedirectSafe。

  if (current_request_uses_header_client_) {
    // 使用我们从OnHeadersReceived获得的标头，因为它将包含。
    // Set-Cookie(如果存在)。
    auto saved_headers = current_response_->headers;
    current_response_ = std::move(head);
    // 如果此重定向来自HSTS升级，则OnHeadersReceived将不会。
    // 在OnReceiveRedirect之前调用，因此请确保保存的标头存在。
    // 在设置它们之前。
    if (saved_headers)
      current_response_->headers = saved_headers;
    ContinueToBeforeRedirect(redirect_info, net::OK);
  } else {
    current_response_ = std::move(head);
    HandleResponseOrRedirectHeaders(
        base::BindOnce(&InProgressRequest::ContinueToBeforeRedirect,
                       weak_factory_.GetWeakPtr(), redirect_info));
  }
}

void ProxyingURLLoaderFactory::InProgressRequest::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {
  target_client_->OnUploadProgress(current_position, total_size,
                                   std::move(callback));
}

void ProxyingURLLoaderFactory::InProgressRequest::OnReceiveCachedMetadata(
    mojo_base::BigBuffer data) {
  target_client_->OnReceiveCachedMetadata(std::move(data));
}

void ProxyingURLLoaderFactory::InProgressRequest::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  target_client_->OnTransferSizeUpdated(transfer_size_diff);
}

void ProxyingURLLoaderFactory::InProgressRequest::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  target_client_->OnStartLoadingResponseBody(std::move(body));
}

void ProxyingURLLoaderFactory::InProgressRequest::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  if (status.error_code != net::OK) {
    OnRequestError(status);
    return;
  }

  target_client_->OnComplete(status);
  factory_->web_request_api()->OnCompleted(&info_.value(), request_,
                                           status.error_code);

  // 删除|此|。
  factory_->RemoveRequest(network_service_request_id_, request_id_);
}

bool ProxyingURLLoaderFactory::IsForServiceWorkerScript() const {
  return loader_factory_type_ == content::ContentBrowserClient::
                                     URLLoaderFactoryType::kServiceWorkerScript;
}

void ProxyingURLLoaderFactory::InProgressRequest::OnLoaderCreated(
    mojo::PendingReceiver<network::mojom::TrustedHeaderClient> receiver) {
  // 涉及CORS时，可能有多个网络：：URLLoader关联。
  // 使用此InProgressRequest，因为CorsURLLoader可能会创建一个新的。
  // 重定向处理中相同请求ID的Network：：URLLoader-请参见。
  // CorsURLLoader：：FollowRedirect。在这种情况下，旧的Network：：URLLoader。
  // 很快就会分离，所以我们不需要处理它。
  // 我们需要这种显式重置，以避免mojo：：Receiver中的DCHECK失败。
  header_client_receiver_.reset();

  header_client_receiver_.Bind(std::move(receiver));
  if (for_cors_preflight_) {
    // 在本例中，我们没有|target_loader_|和。
    // |PROXED_CLIENT_RECEIVER_|，和|RECEIVER|是唯一连接到。
    // 网络服务，因此我们观察到MOJO连接错误。
    header_client_receiver_.set_disconnect_handler(base::BindOnce(
        &ProxyingURLLoaderFactory::InProgressRequest::OnRequestError,
        weak_factory_.GetWeakPtr(),
        network::URLLoaderCompletionStatus(net::ERR_FAILED)));
  }
}

void ProxyingURLLoaderFactory::InProgressRequest::OnBeforeSendHeaders(
    const net::HttpRequestHeaders& headers,
    OnBeforeSendHeadersCallback callback) {
  if (!current_request_uses_header_client_) {
    std::move(callback).Run(net::OK, absl::nullopt);
    return;
  }

  request_.headers = headers;
  on_before_send_headers_callback_ = std::move(callback);
  ContinueToBeforeSendHeaders(net::OK);
}

void ProxyingURLLoaderFactory::InProgressRequest::OnHeadersReceived(
    const std::string& headers,
    const net::IPEndPoint& remote_endpoint,
    OnHeadersReceivedCallback callback) {
  if (!current_request_uses_header_client_) {
    std::move(callback).Run(net::OK, absl::nullopt, GURL());

    if (for_cors_preflight_) {
      // 仅当指定ExtraHeaders时，才支持CORS预检。
      // 删除|此|。
      factory_->RemoveRequest(network_service_request_id_, request_id_);
    }
    return;
  }

  on_headers_received_callback_ = std::move(callback);
  current_response_ = network::mojom::URLResponseHead::New();
  current_response_->headers =
      base::MakeRefCounted<net::HttpResponseHeaders>(headers);
  current_response_->remote_endpoint = remote_endpoint;
  HandleResponseOrRedirectHeaders(
      base::BindOnce(&InProgressRequest::ContinueToHandleOverrideHeaders,
                     weak_factory_.GetWeakPtr()));
}

void ProxyingURLLoaderFactory::InProgressRequest::
    HandleBeforeRequestRedirect() {
  // 扩展请求重定向。关闭与当前。
  // URLLoader并通知URLLoaderClient WebRequest API生成了。
  // 重定向。要加载|redirect_url_|，将重新创建一个新的URLLoader。
  // 收到FollowRedirect()后。

  // 忘记关闭与当前URLLoader的连接导致。
  // 虫子。后者对重定向一无所知。持续。
  // 它的负载产生了意想不到的结果。看见。
  // Https://crbug.com/882661#c72.。
  proxied_client_receiver_.reset();
  header_client_receiver_.reset();
  target_loader_.reset();

  constexpr int kInternalRedirectStatusCode = net::HTTP_TEMPORARY_REDIRECT;

  net::RedirectInfo redirect_info;
  redirect_info.status_code = kInternalRedirectStatusCode;
  redirect_info.new_method = request_.method;
  redirect_info.new_url = redirect_url_;
  redirect_info.new_site_for_cookies =
      net::SiteForCookies::FromUrl(redirect_url_);

  auto head = network::mojom::URLResponseHead::New();
  std::string headers = base::StringPrintf(
      "HTTP/1.1 NaN Internal Redirect\n"
      "Location: %s\n"
      "Non-Authoritative-Reason: WebRequest API\n\n",
      kInternalRedirectStatusCode, redirect_url_.spec().c_str());

  // CorsURLLoader将|REQUEST_INITIATOR|设置为中的源请求头。
  // NetworkService，我们需要在此处修改|REQUEST_INITIATOR|以创建。
  // 间接来源标头。
  // 下面的检查实现了步骤10的“4.4.HTTP-重定向获取(REDIREDIRECT FETCH)”，
  // Https://fetch.spec.whatwg.org/#http-redirect-fetch。
  // 重置启动器以假装已设置规范的受污染来源标志。
  if (request_.request_initiator &&
      (!url::Origin::Create(redirect_url_)
            .IsSameOriginWith(url::Origin::Create(request_.url)) &&
       !request_.request_initiator->IsSameOriginWith(
           url::Origin::Create(request_.url)))) {
    // CORS印前检查不支持重定向。
    request_.request_initiator = url::Origin();
  }
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(headers));
  head->encoded_data_length = 0;

  current_response_ = std::move(head);
  ContinueToBeforeRedirect(redirect_info, net::OK);
}

void ProxyingURLLoaderFactory::InProgressRequest::ContinueToBeforeSendHeaders(
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (!current_request_uses_header_client_ && !redirect_url_.is_empty()) {
    if (for_cors_preflight_) {
      // 注意：在电子中，所有协议都调用onBeforeSendHeaders。
      OnRequestError(network::URLLoaderCompletionStatus(net::ERR_FAILED));
      return;
    }
    HandleBeforeRequestRedirect();
    return;
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();

  auto continuation = base::BindRepeating(
      &InProgressRequest::ContinueToSendHeaders, weak_factory_.GetWeakPtr());
  // 请求已同步取消。发送错误通知。
  int result = factory_->web_request_api()->OnBeforeSendHeaders(
      &info_.value(), request_, continuation, &request_.headers);

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    // 并终止该请求。
    // 一个或多个监听器正在阻塞，因此请求必须暂停到。
    OnRequestError(network::URLLoaderCompletionStatus(result));
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    // 他们会做出回应。|Continuation|以上将被异步调用。
    // 继续或取消请求。
    // 
    // 我们在这里暂停接收器，以防止进一步的客户端消息处理。
    // 对于CORS印前检查请求，我们已经在。
    if (proxied_client_receiver_.is_bound())
      proxied_client_receiver_.Resume();
    return;
  }
  DCHECK_EQ(net::OK, result);

  ContinueToSendHeaders(std::set<std::string>(), std::set<std::string>(),
                        net::OK);
}

void ProxyingURLLoaderFactory::InProgressRequest::ContinueToStartRequest(
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (current_request_uses_header_client_ && !redirect_url_.is_empty()) {
    HandleBeforeRequestRedirect();
    return;
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();

  if (header_client_receiver_.is_bound())
    header_client_receiver_.Resume();

  if (for_cors_preflight_) {
    // 网络服务。我们确实通过阻止请求阻止了该请求。
    // |HEADER_CLIENT_RECEIVER_|，我们在上面解封。
    // 到目前为止还没有延期取消，所以现在可以。
    return;
  }

  if (!target_loader_.is_bound() && factory_->target_factory_.is_bound()) {
    // 发起真实网络请求。
    // 即使此请求不使用头客户端，将来也会重定向。
    uint32_t options = options_;
    // 可能，所以我们需要在装载机上设置选项。
    // 从这里开始，此请求的生命周期由。
    if (has_any_extra_headers_listeners_)
      options |= network::mojom::kURLLoadOptionUseHeaderClient;
    factory_->target_factory_->CreateLoaderAndStart(
        mojo::MakeRequest(&target_loader_), network_service_request_id_,
        options, request_, proxied_client_receiver_.BindNewPipeAndPassRemote(),
        traffic_annotation_);
  }

  // |PROXED_LOADER_RECEIVER_|、|PROXED_CLIENT_RECEIVER_|或。
  // |Header_Client_Receiver_|。
  // 注意：在电子中，所有协议都调用onSendHeaders。
}

void ProxyingURLLoaderFactory::InProgressRequest::ContinueToSendHeaders(
    const std::set<std::string>& removed_headers,
    const std::set<std::string>& set_headers,
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  if (current_request_uses_header_client_) {
    DCHECK(on_before_send_headers_callback_);
    std::move(on_before_send_headers_callback_)
        .Run(error_code, request_.headers);
  } else if (pending_follow_redirect_params_) {
    pending_follow_redirect_params_->removed_headers.insert(
        pending_follow_redirect_params_->removed_headers.end(),
        removed_headers.begin(), removed_headers.end());

    for (auto& set_header : set_headers) {
      std::string header_value;
      if (request_.headers.GetHeader(set_header, &header_value)) {
        pending_follow_redirect_params_->modified_headers.SetHeader(
            set_header, header_value);
      } else {
        NOTREACHED();
      }
    }

    if (target_loader_.is_bound()) {
      target_loader_->FollowRedirect(
          pending_follow_redirect_params_->removed_headers,
          pending_follow_redirect_params_->modified_headers,
          pending_follow_redirect_params_->modified_cors_exempt_headers,
          pending_follow_redirect_params_->new_url);
    }

    pending_follow_redirect_params_.reset();
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();

  // 确保更新CURRENT_RESPONSE_，因为当OnReceiveResponse。
  factory_->web_request_api()->OnSendHeaders(&info_.value(), request_,
                                             request_.headers);

  if (!current_request_uses_header_client_)
    ContinueToStartRequest(net::OK);
}

void ProxyingURLLoaderFactory::InProgressRequest::
    ContinueToHandleOverrideHeaders(int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  DCHECK(on_headers_received_callback_);
  absl::optional<std::string> headers;
  if (override_headers_) {
    headers = override_headers_->raw_headers();
    if (current_request_uses_header_client_) {
      // 调用时，我们将不使用它的标头，因为它可能会丢失。
      // Set-Cookie行(因为它在IPC上被剥离)。
      // 如果这是CORS印前检查，则没有关联的客户端。
      current_response_->headers = override_headers_;
    }
  }

  if (for_cors_preflight_ && !redirect_url_.is_empty()) {
    OnRequestError(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  std::move(on_headers_received_callback_).Run(net::OK, headers, redirect_url_);
  override_headers_ = nullptr;

  if (for_cors_preflight_) {
    // 请勿完成需要代理身份验证的代理印前检查请求。
    info_->AddResponseInfoFromResourceResponse(*current_response_);
    // 请求尚未完成，请将控制权交还给网络服务。
    // 这将启动身份验证过程。
    // 我们在此通知完成，并删除|this|。
    if (info_->response_code == net::HTTP_PROXY_AUTHENTICATION_REQUIRED)
      return;
    // 响应标头可能已被|onHeadersReceired|覆盖。
    factory_->web_request_api()->OnResponseStarted(&info_.value(), request_);
    factory_->web_request_api()->OnCompleted(&info_.value(), request_, net::OK);

    factory_->RemoveRequest(network_service_request_id_, request_id_);
    return;
  }

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();
}

void ProxyingURLLoaderFactory::InProgressRequest::ContinueToResponseStarted(
    int error_code) {
  DCHECK(!for_cors_preflight_);
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  DCHECK(!current_request_uses_header_client_ || !override_headers_);
  if (override_headers_)
    current_response_->headers = override_headers_;

  std::string redirect_location;
  if (override_headers_ && override_headers_->IsRedirect(&redirect_location)) {
    // 处理程序，并且可能已更改为重定向。我们在这里处理这件事。
    // 而不是表现为常规的请求完成。
    // 
    // 请注意，我们实际上不能更改网络服务处理。
    // 在这一点上的原始请求，所以我们的“重定向”实际上只是。
    // 生成人工|onBeforeRedirect|事件并启动新的。
    // 请求网络服务。我们的客户不应该知道有什么不同。
    // 如果由发起新请求，这些将重新绑定。
    GURL new_url(redirect_location);

    net::RedirectInfo redirect_info;
    redirect_info.status_code = override_headers_->response_code();
    redirect_info.new_method = request_.method;
    redirect_info.new_url = new_url;
    redirect_info.new_site_for_cookies = net::SiteForCookies::FromUrl(new_url);

    // |FollowRedirect()。
    // 请求方法可以修改为GET。在这种情况下，我们需要。
    proxied_client_receiver_.reset();
    header_client_receiver_.reset();
    target_loader_.reset();

    ContinueToBeforeRedirect(redirect_info, net::OK);
    return;
  }

  info_->AddResponseInfoFromResourceResponse(*current_response_);

  proxied_client_receiver_.Resume();

  factory_->web_request_api()->OnResponseStarted(&info_.value(), request_);
  target_client_->OnReceiveResponse(current_response_.Clone());
}

void ProxyingURLLoaderFactory::InProgressRequest::ContinueToBeforeRedirect(
    const net::RedirectInfo& redirect_info,
    int error_code) {
  if (error_code != net::OK) {
    OnRequestError(network::URLLoaderCompletionStatus(error_code));
    return;
  }

  info_->AddResponseInfoFromResourceResponse(*current_response_);

  if (proxied_client_receiver_.is_bound())
    proxied_client_receiver_.Resume();

  factory_->web_request_api()->OnBeforeRedirect(&info_.value(), request_,
                                                redirect_info.new_url);
  target_client_->OnReceiveRedirect(redirect_info, current_response_.Clone());
  request_.url = redirect_info.new_url;
  request_.method = redirect_info.new_method;
  request_.site_for_cookies = redirect_info.new_site_for_cookies;
  request_.referrer = GURL(redirect_info.new_referrer);
  request_.referrer_policy = redirect_info.new_referrer_policy;

  // 手动重置请求正文。
  // 一个或多个监听器正在阻塞，因此请求必须暂停到。
  if (request_.method == net::HttpRequestHeaders::kGetMethod)
    request_.request_body = nullptr;
}

void ProxyingURLLoaderFactory::InProgressRequest::
    HandleResponseOrRedirectHeaders(net::CompletionOnceCallback continuation) {
  override_headers_ = nullptr;
  redirect_url_ = GURL();

  info_->AddResponseInfoFromResourceResponse(*current_response_);

  auto callback_pair = base::SplitOnceCallback(std::move(continuation));
  DCHECK(info_.has_value());
  int result = factory_->web_request_api()->OnHeadersReceived(
      &info_.value(), request_, std::move(callback_pair.first),
      current_response_->headers.get(), &override_headers_, &redirect_url_);
  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    OnRequestError(network::URLLoaderCompletionStatus(result));
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    // 他们会做出回应。|Continuation|以上将被异步调用。
    // 继续或取消请求。
    // 
    // 我们在这里暂停接收器，以防止进一步的客户端消息处理。
    // 删除|此|。
    if (proxied_client_receiver_.is_bound())
      proxied_client_receiver_.Pause();
    return;
  }

  DCHECK_EQ(net::OK, result);

  std::move(callback_pair.second).Run(net::OK);
}

void ProxyingURLLoaderFactory::InProgressRequest::OnRequestError(
    const network::URLLoaderCompletionStatus& status) {
  if (target_client_)
    target_client_->OnComplete(status);
  factory_->web_request_api()->OnErrorOccurred(&info_.value(), request_,
                                               status.error_code);

  // 拿一份复印件，这样我们就可以改变要求了。
  factory_->RemoveRequest(network_service_request_id_, request_id_);
}

ProxyingURLLoaderFactory::ProxyingURLLoaderFactory(
    WebRequestAPI* web_request_api,
    const HandlersMap& intercepted_handlers,
    int render_process_id,
    int frame_routing_id,
    int view_routing_id,
    uint64_t* request_id_generator,
    std::unique_ptr<extensions::ExtensionNavigationUIData> navigation_ui_data,
    absl::optional<int64_t> navigation_id,
    network::mojom::URLLoaderFactoryRequest loader_request,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory_remote,
    mojo::PendingReceiver<network::mojom::TrustedURLLoaderHeaderClient>
        header_client_receiver,
    content::ContentBrowserClient::URLLoaderFactoryType loader_factory_type)
    : web_request_api_(web_request_api),
      intercepted_handlers_(intercepted_handlers),
      render_process_id_(render_process_id),
      frame_routing_id_(frame_routing_id),
      view_routing_id_(view_routing_id),
      request_id_generator_(request_id_generator),
      navigation_ui_data_(std::move(navigation_ui_data)),
      navigation_id_(std::move(navigation_id)),
      loader_factory_type_(loader_factory_type) {
  target_factory_.Bind(std::move(target_factory_remote));
  target_factory_.set_disconnect_handler(base::BindOnce(
      &ProxyingURLLoaderFactory::OnTargetFactoryError, base::Unretained(this)));
  proxy_receivers_.Add(this, std::move(loader_request));
  proxy_receivers_.set_disconnect_handler(base::BindRepeating(
      &ProxyingURLLoaderFactory::OnProxyBindingError, base::Unretained(this)));

  if (header_client_receiver)
    url_loader_header_client_receiver_.Bind(std::move(header_client_receiver));

  ignore_connections_limit_domains_ = base::SplitString(
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kIgnoreConnectionsLimit),
      ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
}

bool ProxyingURLLoaderFactory::ShouldIgnoreConnectionsLimit(
    const network::ResourceRequest& request) {
  for (const auto& domain : ignore_connections_limit_domains_) {
    if (request.url.DomainIs(domain)) {
      return true;
    }
  }
  return false;
}

void ProxyingURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& original_request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  // 检查用户是否已拦截此方案。
  network::ResourceRequest request = original_request;

  if (ShouldIgnoreConnectionsLimit(request)) {
    request.priority = net::RequestPriority::MAXIMUM_PRIORITY;
    request.load_flags |= net::LOAD_IGNORE_LIMITS;
  }

  // &lt;方案，&lt;类型，处理程序&gt;&gt;。
  auto it = intercepted_handlers_.find(request.url.scheme());
  if (it != intercepted_handlers_.end()) {
    mojo::PendingRemote<network::mojom::URLLoaderFactory> loader_remote;
    this->Clone(loader_remote.InitWithNewPipeAndPassReceiver());

    // ServiceWorker的加载器禁止从file：//urls加载脚本，并且。
    it->second.second.Run(
        request, base::BindOnce(&ElectronURLLoaderFactory::StartLoading,
                                std::move(loader), request_id, options, request,
                                std::move(client), traffic_annotation,
                                std::move(loader_remote), it->second.first));
    return;
  }

  // 铬不提供覆盖此行为的方法。所以为了。
  // 要使ServiceWorker使用file：//URL，我们必须拦截其。
  // 这里有要求。
  // 通向原来的工厂。
  if (IsForServiceWorkerScript() && request.url.SchemeIsFile()) {
    asar::CreateAsarURLLoader(
        request, std::move(loader), std::move(client),
        base::MakeRefCounted<net::HttpResponseHeaders>(""));
    return;
  }

  if (!web_request_api()->HasListener()) {
    // 请求ID实际上并不重要。它只需要是独一无二的。
    target_factory_->CreateLoaderAndStart(std::move(loader), request_id,
                                          options, request, std::move(client),
                                          traffic_annotation);
    return;
  }

  // 每个BrowserContext，以便扩展可以理解它。请注意。
  // |network_service_request_id_|相比之下，不一定是唯一的，所以我们。
  // 在这里不要用它来证明身份。
  // 注意：Chrome假设ID为零的请求永远不会使用。
  const uint64_t web_request_id = ++(*request_id_generator_);

  // “Extra Headers”代码路径，但在电子请求中开始于。
  // Net模块将具有零ID，因为它们没有渲染器进程。
  // 关联。
  // 请注意，URLLoader现在正在启动，不需要等待。
  if (request_id)
    network_request_id_to_web_request_id_.emplace(request_id, web_request_id);

  auto result = requests_.emplace(
      web_request_id,
      std::make_unique<InProgressRequest>(
          this, web_request_id, view_routing_id_, frame_routing_id_, request_id,
          options, request, traffic_annotation, std::move(loader),
          std::move(client)));
  result.first->second->Restart();
}

void ProxyingURLLoaderFactory::Clone(
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> loader_receiver) {
  proxy_receivers_.Add(this, std::move(loader_receiver));
}

void ProxyingURLLoaderFactory::OnLoaderCreated(
    int32_t request_id,
    mojo::PendingReceiver<network::mojom::TrustedHeaderClient> receiver) {
  auto it = network_request_id_to_web_request_id_.find(request_id);
  if (it == network_request_id_to_web_request_id_.end())
    return;

  auto request_it = requests_.find(it->second);
  DCHECK(request_it != requests_.end());
  request_it->second->OnLoaderCreated(std::move(receiver));
}

void ProxyingURLLoaderFactory::OnLoaderForCorsPreflightCreated(
    const network::ResourceRequest& request,
    mojo::PendingReceiver<network::mojom::TrustedHeaderClient> receiver) {
  // 从这里发来的附加信号。URLLoader在此之前将被阻止。
  // 发送HTTP请求头(Trust dHeaderClient.OnBeforeSendHeaders)，
  // 但连接设置将在此之前完成。从以下方面来看，这是可以接受的。
  // Web请求API，因为扩展已允许设置。
  // 连接到相同的URL(即，实际请求)，并区分。
  // 实际请求和之前的印前检查请求的两个连接。
  // 发送请求头非常困难。
  // 即使连接到此对象的所有URLLoaderFactory管道。
  const uint64_t web_request_id = ++(*request_id_generator_);

  auto result = requests_.insert(std::make_pair(
      web_request_id, std::make_unique<InProgressRequest>(
                          this, web_request_id, frame_routing_id_, request)));

  result.first->second->OnLoaderCreated(std::move(receiver));
  result.first->second->Restart();
}

ProxyingURLLoaderFactory::~ProxyingURLLoaderFactory() = default;

void ProxyingURLLoaderFactory::OnTargetFactoryError() {
  target_factory_.reset();
  proxy_receivers_.Clear();

  MaybeDeleteThis();
}

void ProxyingURLLoaderFactory::OnProxyBindingError() {
  if (proxy_receivers_.empty())
    target_factory_.reset();

  MaybeDeleteThis();
}

void ProxyingURLLoaderFactory::RemoveRequest(int32_t network_service_request_id,
                                             uint64_t request_id) {
  network_request_id_to_web_request_id_.erase(network_service_request_id);
  requests_.erase(request_id);

  MaybeDeleteThis();
}

void ProxyingURLLoaderFactory::MaybeDeleteThis() {
  // 关闭它必须保持活动状态，直到所有活动请求都完成。
  // 命名空间电子
  if (target_factory_.is_bound() || !requests_.empty() ||
      !proxy_receivers_.empty())
    return;

  delete this;
}

}  //%s
