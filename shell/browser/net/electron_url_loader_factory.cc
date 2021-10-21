// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/net/electron_url_loader_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "mojo/public/cpp/system/string_data_source.h"
#include "net/base/filename_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/redirect_util.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/net/asar/asar_url_loader.h"
#include "shell/browser/net/node_stream_loader.h"
#include "shell/browser/net/url_pipe_loader.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

#include "shell/common/node_includes.h"

using content::BrowserThread;

namespace gin {

template <>
struct Converter<electron::ProtocolType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::ProtocolType* out) {
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "buffer")
      *out = electron::ProtocolType::kBuffer;
    else if (type == "string")
      *out = electron::ProtocolType::kString;
    else if (type == "file")
      *out = electron::ProtocolType::kFile;
    else if (type == "http")
      *out = electron::ProtocolType::kHttp;
    else if (type == "stream")
      *out = electron::ProtocolType::kStream;
    else  // 注意“free”为内部类型，用户不允许传入。
      return false;
    return true;
  }
};

}  // 命名空间杜松子酒。

namespace electron {

namespace {

// 确定协议类型是否可以接受非对象响应。
bool ResponseMustBeObject(ProtocolType type) {
  switch (type) {
    case ProtocolType::kString:
    case ProtocolType::kFile:
    case ProtocolType::kFree:
      return false;
    default:
      return true;
  }
}

// 用于将值转换为字典的帮助器。
gin::Dictionary ToDict(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  if (!value->IsFunction() && value->IsObject())
    return gin::Dictionary(
        isolate,
        value->ToObject(isolate->GetCurrentContext()).ToLocalChecked());
  else
    return gin::Dictionary(isolate);
}

// 从响应对象解析标头。
network::mojom::URLResponseHeadPtr ToResponseHead(
    const gin_helper::Dictionary& dict) {
  auto head = network::mojom::URLResponseHead::New();
  head->mime_type = "text/html";
  head->charset = "utf-8";
  if (dict.IsEmpty()) {
    head->headers =
        base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
    return head;
  }

  int status_code = net::HTTP_OK;
  dict.Get("statusCode", &status_code);
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      base::StringPrintf("HTTP/1.1 %d %s", status_code,
                         net::GetHttpReasonPhrase(
                             static_cast<net::HttpStatusCode>(status_code))));

  dict.Get("charset", &head->charset);
  bool has_mime_type = dict.Get("mimeType", &head->mime_type);
  bool has_content_type = false;

  base::DictionaryValue headers;
  if (dict.Get("headers", &headers)) {
    for (const auto iter : headers.DictItems()) {
      if (iter.second.is_string()) {
        // 键、值。
        head->headers->AddHeader(iter.first, iter.second.GetString());
      } else if (iter.second.is_list()) {
        // 密钥：[值...]。
        for (const auto& item : iter.second.GetList()) {
          if (item.is_string())
            head->headers->AddHeader(iter.first, item.GetString());
        }
      } else {
        continue;
      }
      auto header_name_lowercase = base::ToLowerASCII(iter.first);

      if (header_name_lowercase == "content-type") {
        // 一些应用程序通过标头传递内容类型，这是不被接受的。
        // 在网络服务中。
        head->headers->GetMimeTypeAndCharset(&head->mime_type, &head->charset);
        has_content_type = true;
      } else if (header_name_lowercase == "content-length" &&
                 iter.second.is_string()) {
        base::StringToInt64(iter.second.GetString(), &head->content_length);
      }
    }
  }

  // 设置|head-&gt;MIME_TYPE|不会自动设置“Content-type”
  // NetworkService中的标头。
  if (has_mime_type && !has_content_type)
    head->headers->AddHeader("content-type", head->mime_type);
  return head;
}

// 将字符串写入管道的帮助器。
struct WriteData {
  mojo::Remote<network::mojom::URLLoaderClient> client;
  std::string data;
  std::unique_ptr<mojo::DataPipeProducer> producer;
};

void OnWrite(std::unique_ptr<WriteData> write_data, MojoResult result) {
  network::URLLoaderCompletionStatus status(net::ERR_FAILED);
  if (result == MOJO_RESULT_OK) {
    status = network::URLLoaderCompletionStatus(net::OK);
    status.encoded_data_length = write_data->data.size();
    status.encoded_body_length = write_data->data.size();
    status.decoded_body_length = write_data->data.size();
  }
  write_data->client->OnComplete(status);
}

}  // 命名空间。

ElectronURLLoaderFactory::RedirectedRequest::RedirectedRequest(
    const net::RedirectInfo& redirect_info,
    mojo::PendingReceiver<network::mojom::URLLoader> loader_receiver,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory_remote)
    : redirect_info_(redirect_info),
      request_id_(request_id),
      options_(options),
      request_(request),
      client_(std::move(client)),
      traffic_annotation_(traffic_annotation) {
  loader_receiver_.Bind(std::move(loader_receiver));
  loader_receiver_.set_disconnect_handler(
      base::BindOnce(&ElectronURLLoaderFactory::RedirectedRequest::DeleteThis,
                     base::Unretained(this)));
  target_factory_remote_.Bind(std::move(target_factory_remote));
  target_factory_remote_.set_disconnect_handler(base::BindOnce(
      &ElectronURLLoaderFactory::RedirectedRequest::OnTargetFactoryError,
      base::Unretained(this)));
}

ElectronURLLoaderFactory::RedirectedRequest::~RedirectedRequest() = default;

void ElectronURLLoaderFactory::RedirectedRequest::FollowRedirect(
    const std::vector<std::string>& removed_headers,
    const net::HttpRequestHeaders& modified_headers,
    const net::HttpRequestHeaders& modified_cors_exempt_headers,
    const absl::optional<GURL>& new_url) {
  // 使用来自重定向的信息更新|REQUEST_|，以确保其准确性。
  // 以下引用WorkerScriptLoader：：FollowRedirect中的代码。
  bool should_clear_upload = false;
  net::RedirectUtil::UpdateHttpRequest(
      request_.url, request_.method, redirect_info_, removed_headers,
      modified_headers, &request_.headers, &should_clear_upload);
  request_.cors_exempt_headers.MergeFrom(modified_cors_exempt_headers);
  for (const std::string& name : removed_headers)
    request_.cors_exempt_headers.RemoveHeader(name);

  if (should_clear_upload)
    request_.request_body = nullptr;

  request_.url = redirect_info_.new_url;
  request_.method = redirect_info_.new_method;
  request_.site_for_cookies = redirect_info_.new_site_for_cookies;
  request_.referrer = GURL(redirect_info_.new_referrer);
  request_.referrer_policy = redirect_info_.new_referrer_policy;

  // 创建新的加载器以处理重定向并销毁此加载器。
  target_factory_remote_->CreateLoaderAndStart(
      loader_receiver_.Unbind(), request_id_, options_, request_,
      std::move(client_), traffic_annotation_);

  DeleteThis();
}

void ElectronURLLoaderFactory::RedirectedRequest::OnTargetFactoryError() {
  // 此时无法创建新的加载器，因此请求无法继续。
  mojo::Remote<network::mojom::URLLoaderClient> client_remote(
      std::move(client_));
  client_remote->OnComplete(
      network::URLLoaderCompletionStatus(net::ERR_FAILED));
  client_remote.reset();

  DeleteThis();
}

void ElectronURLLoaderFactory::RedirectedRequest::DeleteThis() {
  loader_receiver_.reset();
  target_factory_remote_.reset();

  delete this;
}

// 静电。
mojo::PendingRemote<network::mojom::URLLoaderFactory>
ElectronURLLoaderFactory::Create(ProtocolType type,
                                 const ProtocolHandler& handler) {
  mojo::PendingRemote<network::mojom::URLLoaderFactory> pending_remote;

  // 当没有更多文件时，ElectronURLLoaderFactory将自动删除。
  // 接收器-请参见SelfDeletingURLLoaderFactory：：OnDisconnect方法。
  new ElectronURLLoaderFactory(type, handler,
                               pending_remote.InitWithNewPipeAndPassReceiver());

  return pending_remote;
}

ElectronURLLoaderFactory::ElectronURLLoaderFactory(
    ProtocolType type,
    const ProtocolHandler& handler,
    mojo::PendingReceiver<network::mojom::URLLoaderFactory> factory_receiver)
    : network::SelfDeletingURLLoaderFactory(std::move(factory_receiver)),
      type_(type),
      handler_(handler) {}

ElectronURLLoaderFactory::~ElectronURLLoaderFactory() = default;

void ElectronURLLoaderFactory::CreateLoaderAndStart(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // |StartLoding|用于拦截和注册的协议。
  // 在重定向上，它需要一个工厂来为。
  // 新请求。所以在这种情况下，这个工厂就是目标工厂。
  mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory;
  this->Clone(target_factory.InitWithNewPipeAndPassReceiver());

  handler_.Run(
      request,
      base::BindOnce(&ElectronURLLoaderFactory::StartLoading, std::move(loader),
                     request_id, options, request, std::move(client),
                     traffic_annotation, std::move(target_factory), type_));
}

// 静电。
void ElectronURLLoaderFactory::OnComplete(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    int32_t request_id,
    const network::URLLoaderCompletionStatus& status) {
  if (client.is_valid()) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(status);
  }
}

// 静电。
void ElectronURLLoaderFactory::StartLoading(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojo::PendingRemote<network::mojom::URLLoaderFactory> target_factory,
    ProtocolType type,
    gin::Arguments* args) {
  // 未传递参数时发送网络错误。
  // 
  // 请注意，无论是什么，我们都不应该在回调中抛出JS错误。
  // 通过，以保持与旧代码的兼容性。
  v8::Local<v8::Value> response;
  if (!args->GetNext(&response)) {
    OnComplete(std::move(client), request_id,
               network::URLLoaderCompletionStatus(net::ERR_NOT_IMPLEMENTED));
    return;
  }

  // 分析{Error}对象。
  gin_helper::Dictionary dict = ToDict(args->isolate(), response);
  if (!dict.IsEmpty()) {
    int error_code;
    if (dict.Get("error", &error_code)) {
      OnComplete(std::move(client), request_id,
                 network::URLLoaderCompletionStatus(error_code));
      return;
    }
  }

  network::mojom::URLResponseHeadPtr head = ToResponseHead(dict);

  // 处理重定向。
  // 
  // 请注意，使用NetworkService时，不再发送“Location”报头。
  // 自动重定向请求，我们显式创建了一个新的加载器。
  // 若要实现重定向，请执行以下操作。这也是Chromium对WebRequest所做的事情。
  // WebRequestProxyingURLLoaderFactory中的接口。
  std::string location;
  if (head->headers->IsRedirect(&location)) {
    // 如果请求是Main_Frame请求，则第一方URL将获取。
    // 已更新重定向。
    const net::RedirectInfo::FirstPartyURLPolicy first_party_url_policy =
        request.resource_type ==
                static_cast<int>(blink::mojom::ResourceType::kMainFrame)
            ? net::RedirectInfo::FirstPartyURLPolicy::UPDATE_URL_ON_REDIRECT
            : net::RedirectInfo::FirstPartyURLPolicy::NEVER_CHANGE_URL;

    net::RedirectInfo redirect_info = net::RedirectInfo::ComputeRedirectInfo(
        request.method, request.url, request.site_for_cookies,
        first_party_url_policy, request.referrer_policy,
        request.referrer.GetAsReferrer().spec(), head->headers->response_code(),
        request.url.Resolve(location),
        net::RedirectUtil::GetReferrerPolicyHeader(head->headers.get()), false);

    DCHECK(client.is_valid());

    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));

    client_remote->OnReceiveRedirect(redirect_info, std::move(head));

    // 绑定URLLoader接收器并等待FollowRedirect请求，或。
    // 要断开连接的遥控器，如果请求中止，则会发生这种情况。
    // 当重定向到不同的方案时，可能会发生这种情况，这将。
    // 导致销毁URL加载器并使用。
    // 该计划的工厂。
    new RedirectedRequest(redirect_info, std::move(loader), request_id, options,
                          request, client_remote.Unbind(), traffic_annotation,
                          std::move(target_factory));

    return;
  }

  // 一些协议接受非对象响应。
  if (dict.IsEmpty() && ResponseMustBeObject(type)) {
    OnComplete(std::move(client), request_id,
               network::URLLoaderCompletionStatus(net::ERR_NOT_IMPLEMENTED));
    return;
  }

  switch (type) {
    case ProtocolType::kBuffer:
      StartLoadingBuffer(std::move(client), std::move(head), dict);
      break;
    case ProtocolType::kString:
      StartLoadingString(std::move(client), std::move(head), dict,
                         args->isolate(), response);
      break;
    case ProtocolType::kFile:
      StartLoadingFile(std::move(loader), request, std::move(client),
                       std::move(head), dict, args->isolate(), response);
      break;
    case ProtocolType::kHttp:
      StartLoadingHttp(std::move(loader), request, std::move(client),
                       traffic_annotation, dict);
      break;
    case ProtocolType::kStream:
      StartLoadingStream(std::move(loader), std::move(client), std::move(head),
                         dict);
      break;
    case ProtocolType::kFree:
      ProtocolType type;
      if (!gin::ConvertFromV8(args->isolate(), response, &type)) {
        OnComplete(std::move(client), request_id,
                   network::URLLoaderCompletionStatus(net::ERR_FAILED));
        return;
      }
      StartLoading(std::move(loader), request_id, options, request,
                   std::move(client), traffic_annotation,
                   std::move(target_factory), type, args);
      break;
  }
}

// 静电。
void ElectronURLLoaderFactory::StartLoadingBuffer(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    const gin_helper::Dictionary& dict) {
  v8::Local<v8::Value> buffer = dict.GetHandle();
  dict.Get("data", &buffer);
  if (!node::Buffer::HasInstance(buffer)) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  SendContents(
      std::move(client), std::move(head),
      std::string(node::Buffer::Data(buffer), node::Buffer::Length(buffer)));
}

// 静电。
void ElectronURLLoaderFactory::StartLoadingString(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    const gin_helper::Dictionary& dict,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response) {
  std::string contents;
  if (response->IsString()) {
    contents = gin::V8ToString(isolate, response);
  } else if (!dict.IsEmpty()) {
    dict.Get("data", &contents);
  } else {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  SendContents(std::move(client), std::move(head), std::move(contents));
}

// 静电。
void ElectronURLLoaderFactory::StartLoadingFile(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    network::ResourceRequest request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    const gin_helper::Dictionary& dict,
    v8::Isolate* isolate,
    v8::Local<v8::Value> response) {
  base::FilePath path;
  if (gin::ConvertFromV8(isolate, response, &path)) {
    request.url = net::FilePathToFileURL(path);
  } else if (!dict.IsEmpty()) {
    dict.Get("referrer", &request.referrer);
    dict.Get("method", &request.method);
    if (dict.Get("path", &path))
      request.url = net::FilePathToFileURL(path);
  } else {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  // 添加标题以忽略CORS。
  head->headers->AddHeader("Access-Control-Allow-Origin", "*");
  asar::CreateAsarURLLoader(request, std::move(loader), std::move(client),
                            head->headers);
}

// 静电。
void ElectronURLLoaderFactory::StartLoadingHttp(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    const network::ResourceRequest& original_request,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    const gin_helper::Dictionary& dict) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->headers = original_request.headers;
  request->cors_exempt_headers = original_request.cors_exempt_headers;

  dict.Get("url", &request->url);
  dict.Get("referrer", &request->referrer);
  if (!dict.Get("method", &request->method))
    request->method = original_request.method;

  base::DictionaryValue upload_data;
  if (request->method != net::HttpRequestHeaders::kGetMethod &&
      request->method != net::HttpRequestHeaders::kHeadMethod)
    dict.Get("uploadData", &upload_data);

  ElectronBrowserContext* browser_context =
      ElectronBrowserContext::From("", false);
  v8::Local<v8::Value> value;
  if (dict.Get("session", &value)) {
    if (value->IsNull()) {
      browser_context =
          ElectronBrowserContext::From(base::GenerateGUID(), true);
    } else {
      gin::Handle<api::Session> session;
      if (gin::ConvertFromV8(dict.isolate(), value, &session) &&
          !session.IsEmpty()) {
        browser_context = session->browser_context();
      }
    }
  }

  new URLPipeLoader(
      browser_context->GetURLLoaderFactory(), std::move(request),
      std::move(loader), std::move(client),
      static_cast<net::NetworkTrafficAnnotationTag>(traffic_annotation),
      std::move(upload_data));
}

// 静电。
void ElectronURLLoaderFactory::StartLoadingStream(
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    const gin_helper::Dictionary& dict) {
  v8::Local<v8::Value> stream;
  if (!dict.Get("data", &stream)) {
    // 假设OPTS已经是一个流。
    stream = dict.GetHandle();
  } else if (stream->IsNullOrUndefined()) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    // 假定用户需要，“data”被显式作为NULL或未定义传递。
    // 送去一具空身体。
    // 
    // 请注意，我们必须提交一个空正文，否则NetworkService将。
    // 撞车。
    client_remote->OnReceiveResponse(std::move(head));
    mojo::ScopedDataPipeProducerHandle producer;
    mojo::ScopedDataPipeConsumerHandle consumer;
    if (mojo::CreateDataPipe(nullptr, producer, consumer) != MOJO_RESULT_OK) {
      client_remote->OnComplete(
          network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
      return;
    }
    producer.reset();  // 数据管道为空。
    client_remote->OnStartLoadingResponseBody(std::move(consumer));
    client_remote->OnComplete(network::URLLoaderCompletionStatus(net::OK));
    return;
  } else if (!stream->IsObject()) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  gin_helper::Dictionary data = ToDict(dict.isolate(), stream);
  v8::Local<v8::Value> method;
  if (!data.Get("on", &method) || !method->IsFunction() ||
      !data.Get("removeListener", &method) || !method->IsFunction()) {
    mojo::Remote<network::mojom::URLLoaderClient> client_remote(
        std::move(client));
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_FAILED));
    return;
  }

  new NodeStreamLoader(std::move(head), std::move(loader), std::move(client),
                       data.isolate(), data.GetHandle());
}

// 静电。
void ElectronURLLoaderFactory::SendContents(
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    network::mojom::URLResponseHeadPtr head,
    std::string data) {
  mojo::Remote<network::mojom::URLLoaderClient> client_remote(
      std::move(client));

  // 添加标题以忽略CORS。
  head->headers->AddHeader("Access-Control-Allow-Origin", "*");
  client_remote->OnReceiveResponse(std::move(head));

  // 下面的代码遵循data_url_loader_factory.cc模式。
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  if (mojo::CreateDataPipe(nullptr, producer, consumer) != MOJO_RESULT_OK) {
    client_remote->OnComplete(
        network::URLLoaderCompletionStatus(net::ERR_INSUFFICIENT_RESOURCES));
    return;
  }

  client_remote->OnStartLoadingResponseBody(std::move(consumer));

  auto write_data = std::make_unique<WriteData>();
  write_data->client = std::move(client_remote);
  write_data->data = std::move(data);
  write_data->producer =
      std::make_unique<mojo::DataPipeProducer>(std::move(producer));
  auto* producer_ptr = write_data->producer.get();

  base::StringPiece string_piece(write_data->data);
  producer_ptr->Write(
      std::make_unique<mojo::StringDataSource>(
          string_piece, mojo::StringDataSource::AsyncWritingMode::
                            STRING_STAYS_VALID_UNTIL_COMPLETION),
      base::BindOnce(OnWrite, std::move(write_data)));
}

}  // 命名空间电子
