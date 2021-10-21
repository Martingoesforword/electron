// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_url_loader.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/no_destructor.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/chunked_data_pipe_getter.mojom.h"
#include "services/network/public/mojom/http_raw_headers.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace gin {

template <>
struct Converter<network::mojom::HttpRawHeaderPairPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const network::mojom::HttpRawHeaderPairPtr& pair) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.Set("key", base::ToLowerASCII(pair->key));
    dict.Set("value", pair->value);
    return dict.GetHandle();
  }
};

template <>
struct Converter<network::mojom::CredentialsMode> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     network::mojom::CredentialsMode* out) {
    std::string mode;
    if (!ConvertFromV8(isolate, val, &mode))
      return false;
    if (mode == "omit")
      *out = network::mojom::CredentialsMode::kOmit;
    else if (mode == "include")
      *out = network::mojom::CredentialsMode::kInclude;
    else
      // 从技术上讲，“同源”也是这个枚举的成员，但它。
      // 在`net.request()`的上下文中没有意义，因此不要转换。
      // 它。
      return false;
    return true;
  }
};  // 命名空间杜松子酒。

}  // 命名空间杜松子酒。

namespace electron {

namespace api {

namespace {

class BufferDataSource : public mojo::DataPipeProducer::DataSource {
 public:
  explicit BufferDataSource(base::span<char> buffer) {
    buffer_.resize(buffer.size());
    memcpy(buffer_.data(), buffer.data(), buffer_.size());
  }
  ~BufferDataSource() override = default;

 private:
  // MOJO：：DataPipeProducer：：DataSource：
  uint64_t GetLength() const override { return buffer_.size(); }
  ReadResult Read(uint64_t offset, base::span<char> buffer) override {
    ReadResult result;
    if (offset <= buffer_.size()) {
      size_t readable_size = buffer_.size() - offset;
      size_t writable_size = buffer.size();
      size_t copyable_size = std::min(readable_size, writable_size);
      if (copyable_size > 0) {
        memcpy(buffer.data(), &buffer_[offset], copyable_size);
      }
      result.bytes_read = copyable_size;
    } else {
      NOTREACHED();
      result.result = MOJO_RESULT_OUT_OF_RANGE;
    }
    return result;
  }

  std::vector<char> buffer_;
};

class JSChunkedDataPipeGetter : public gin::Wrappable<JSChunkedDataPipeGetter>,
                                public network::mojom::ChunkedDataPipeGetter {
 public:
  static gin::Handle<JSChunkedDataPipeGetter> Create(
      v8::Isolate* isolate,
      v8::Local<v8::Function> body_func,
      mojo::PendingReceiver<network::mojom::ChunkedDataPipeGetter>
          chunked_data_pipe_getter) {
    return gin::CreateHandle(
        isolate, new JSChunkedDataPipeGetter(
                     isolate, body_func, std::move(chunked_data_pipe_getter)));
  }

  // 杜松子酒：：可包装的。
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<JSChunkedDataPipeGetter>::GetObjectTemplateBuilder(
               isolate)
        .SetMethod("write", &JSChunkedDataPipeGetter::WriteChunk)
        .SetMethod("done", &JSChunkedDataPipeGetter::Done);
  }

  static gin::WrapperInfo kWrapperInfo;
  ~JSChunkedDataPipeGetter() override = default;

 private:
  JSChunkedDataPipeGetter(
      v8::Isolate* isolate,
      v8::Local<v8::Function> body_func,
      mojo::PendingReceiver<network::mojom::ChunkedDataPipeGetter>
          chunked_data_pipe_getter)
      : isolate_(isolate), body_func_(isolate, body_func) {
    receiver_.Bind(std::move(chunked_data_pipe_getter));
  }

  // Network：：mojom：：ChunkedDataPipeGetter：
  void GetSize(GetSizeCallback callback) override {
    size_callback_ = std::move(callback);
  }

  void StartReading(mojo::ScopedDataPipeProducerHandle pipe) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    if (body_func_.IsEmpty()) {
      LOG(ERROR) << "Tried to read twice from a JSChunkedDataPipeGetter";
      // 把手柄放在地板上。
      return;
    }
    data_producer_ = std::make_unique<mojo::DataPipeProducer>(std::move(pipe));

    v8::HandleScope handle_scope(isolate_);
    auto maybe_wrapper = GetWrapper(isolate_);
    v8::Local<v8::Value> wrapper;
    if (!maybe_wrapper.ToLocal(&wrapper)) {
      return;
    }
    v8::Local<v8::Value> argv[] = {wrapper};
    node::Environment* env = node::Environment::GetCurrent(isolate_);
    auto global = env->context()->Global();
    node::MakeCallback(isolate_, global, body_func_.Get(isolate_),
                       node::arraysize(argv), argv, {0, 0});
  }

  v8::Local<v8::Promise> WriteChunk(v8::Local<v8::Value> buffer_val) {
    gin_helper::Promise<void> promise(isolate_);
    v8::Local<v8::Promise> handle = promise.GetHandle();
    if (!buffer_val->IsArrayBufferView()) {
      promise.RejectWithErrorMessage("Expected an ArrayBufferView");
      return handle;
    }
    if (is_writing_) {
      promise.RejectWithErrorMessage("Only one write can be pending at a time");
      return handle;
    }
    if (!size_callback_) {
      promise.RejectWithErrorMessage("Can't write after calling done()");
      return handle;
    }
    auto buffer = buffer_val.As<v8::ArrayBufferView>();
    is_writing_ = true;
    bytes_written_ += buffer->ByteLength();
    auto backing_store = buffer->Buffer()->GetBackingStore();
    auto buffer_span = base::make_span(
        static_cast<char*>(backing_store->Data()) + buffer->ByteOffset(),
        buffer->ByteLength());
    auto buffer_source = std::make_unique<BufferDataSource>(buffer_span);
    data_producer_->Write(
        std::move(buffer_source),
        base::BindOnce(&JSChunkedDataPipeGetter::OnWriteChunkComplete,
                       // 我们可以在这里使用Unreposed，因为我们拥有。
                       // |Data_Producer_|。
                       base::Unretained(this), std::move(promise)));
    return handle;
  }

  void OnWriteChunkComplete(gin_helper::Promise<void> promise,
                            MojoResult result) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    is_writing_ = false;
    if (result == MOJO_RESULT_OK) {
      promise.Resolve();
    } else {
      promise.RejectWithErrorMessage("mojo result not ok: " +
                                     std::to_string(result));
      Finished();
    }
  }

  // TODO(Nornagon)：此处接受净错误，以允许数据提供程序。
  // 取消请求，但出现错误。
  void Done() {
    if (size_callback_) {
      std::move(size_callback_).Run(net::OK, bytes_written_);
      Finished();
    }
  }

  void Finished() {
    body_func_.Reset();
    data_producer_.reset();
    receiver_.reset();
    size_callback_.Reset();
  }

  GetSizeCallback size_callback_;
  mojo::Receiver<network::mojom::ChunkedDataPipeGetter> receiver_{this};
  std::unique_ptr<mojo::DataPipeProducer> data_producer_;
  bool is_writing_ = false;
  uint64_t bytes_written_ = 0;

  v8::Isolate* isolate_;
  v8::Global<v8::Function> body_func_;
};

gin::WrapperInfo JSChunkedDataPipeGetter::kWrapperInfo = {
    gin::kEmbedderNativeGin};

const net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("electron_net_module", R"(
        semantics {
          sender: "Electron Net module"
          description:
            "Issue HTTP/HTTPS requests using Chromium's native networking "
            "library."
          trigger: "Using the Net module"
          data: "Anything the user wants to send."
          destination: OTHER
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting: "This feature cannot be disabled."
        })");

}  // 命名空间。

gin::WrapperInfo SimpleURLLoaderWrapper::kWrapperInfo = {
    gin::kEmbedderNativeGin};

SimpleURLLoaderWrapper::SimpleURLLoaderWrapper(
    std::unique_ptr<network::ResourceRequest> request,
    network::mojom::URLLoaderFactory* url_loader_factory,
    int options) {
  if (!request->trusted_params)
    request->trusted_params = network::ResourceRequest::TrustedParams();
  mojo::PendingRemote<network::mojom::URLLoaderNetworkServiceObserver>
      url_loader_network_observer_remote;
  url_loader_network_observer_receivers_.Add(
      this,
      url_loader_network_observer_remote.InitWithNewPipeAndPassReceiver());
  request->trusted_params->url_loader_network_observer =
      std::move(url_loader_network_observer_remote);
  // Chrome使用浏览器规则过滤标题，而对于net模块，我们有。
  // 每个头都通过了。下面的设置将允许我们捕获。
  // URLLoader中的原始标头。
  request->report_raw_headers = true;
  // SimpleURLLoader希望控制请求主体本身。我们还有其他的。
  // 想法。
  auto request_body = std::move(request->request_body);
  auto* request_ref = request.get();
  loader_ =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);
  if (request_body) {
    request_ref->request_body = std::move(request_body);
  }

  loader_->SetAllowHttpErrorResults(true);
  loader_->SetURLLoaderFactoryOptions(options);
  loader_->SetOnResponseStartedCallback(base::BindOnce(
      &SimpleURLLoaderWrapper::OnResponseStarted, base::Unretained(this)));
  loader_->SetOnRedirectCallback(base::BindRepeating(
      &SimpleURLLoaderWrapper::OnRedirect, base::Unretained(this)));
  loader_->SetOnUploadProgressCallback(base::BindRepeating(
      &SimpleURLLoaderWrapper::OnUploadProgress, base::Unretained(this)));
  loader_->SetOnDownloadProgressCallback(base::BindRepeating(
      &SimpleURLLoaderWrapper::OnDownloadProgress, base::Unretained(this)));

  loader_->DownloadAsStream(url_loader_factory, this);
}

void SimpleURLLoaderWrapper::Pin() {
  // 在请求完成之前防止我们自己被GC。一定是。
  // 在gin：：CreateHandle之后调用，否则包装不会初始化。
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  pinned_wrapper_.Reset(isolate, GetWrapper(isolate).ToLocalChecked());
}

void SimpleURLLoaderWrapper::PinBodyGetter(v8::Local<v8::Value> body_getter) {
  pinned_chunk_pipe_getter_.Reset(JavascriptEnvironment::GetIsolate(),
                                  body_getter);
}

SimpleURLLoaderWrapper::~SimpleURLLoaderWrapper() = default;

void SimpleURLLoaderWrapper::OnAuthRequired(
    const absl::optional<base::UnguessableToken>& window_id,
    uint32_t request_id,
    const GURL& url,
    bool first_auth_attempt,
    const net::AuthChallengeInfo& auth_info,
    const scoped_refptr<net::HttpResponseHeaders>& head_headers,
    mojo::PendingRemote<network::mojom::AuthChallengeResponder>
        auth_challenge_responder) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  mojo::Remote<network::mojom::AuthChallengeResponder> auth_responder(
      std::move(auth_challenge_responder));
  // WeakPtr，因为如果我们在等待身份验证时被取消()，并且。
  // 网络服务也决定同时取消并终止此服务。
  // 管道，我们可能会在内存死掉的情况下再次尝试调用Cancel。
  auth_responder.set_disconnect_handler(base::BindOnce(
      &SimpleURLLoaderWrapper::Cancel, weak_factory_.GetWeakPtr()));
  auto cb = base::BindOnce(
      [](mojo::Remote<network::mojom::AuthChallengeResponder> auth_responder,
         gin::Arguments* args) {
        std::u16string username_str, password_str;
        if (!args->GetNext(&username_str) || !args->GetNext(&password_str)) {
          auth_responder->OnAuthCredentials(absl::nullopt);
          return;
        }
        auth_responder->OnAuthCredentials(
            net::AuthCredentials(username_str, password_str));
      },
      std::move(auth_responder));
  Emit("login", auth_info, base::AdaptCallbackForRepeating(std::move(cb)));
}

void SimpleURLLoaderWrapper::OnSSLCertificateError(
    const GURL& url,
    int net_error,
    const net::SSLInfo& ssl_info,
    bool fatal,
    OnSSLCertificateErrorCallback response) {
  std::move(response).Run(net_error);
}

void SimpleURLLoaderWrapper::OnClearSiteData(const GURL& url,
                                             const std::string& header_value,
                                             int32_t load_flags,
                                             OnClearSiteDataCallback callback) {
  std::move(callback).Run();
}
void SimpleURLLoaderWrapper::OnLoadingStateUpdate(
    network::mojom::LoadInfoPtr info,
    OnLoadingStateUpdateCallback callback) {
  std::move(callback).Run();
}

void SimpleURLLoaderWrapper::Clone(
    mojo::PendingReceiver<network::mojom::URLLoaderNetworkServiceObserver>
        observer) {
  url_loader_network_observer_receivers_.Add(this, std::move(observer));
}

void SimpleURLLoaderWrapper::Cancel() {
  loader_.reset();
  pinned_wrapper_.Reset();
  pinned_chunk_pipe_getter_.Reset();
  // 这样可以确保不会调用更多的回调，因此不需要。
  // 需要额外的警卫。
}

// 静电。
gin::Handle<SimpleURLLoaderWrapper> SimpleURLLoaderWrapper::Create(
    gin::Arguments* args) {
  gin_helper::Dictionary opts;
  if (!args->GetNext(&opts)) {
    args->ThrowTypeError("Expected a dictionary");
    return gin::Handle<SimpleURLLoaderWrapper>();
  }
  auto request = std::make_unique<network::ResourceRequest>();
  opts.Get("method", &request->method);
  opts.Get("url", &request->url);
  request->site_for_cookies = net::SiteForCookies::FromUrl(request->url);
  opts.Get("referrer", &request->referrer);
  std::string origin;
  opts.Get("origin", &origin);
  if (!origin.empty()) {
    request->request_initiator = url::Origin::Create(GURL(origin));
  }
  bool has_user_activation;
  if (opts.Get("hasUserActivation", &has_user_activation)) {
    request->trusted_params = network::ResourceRequest::TrustedParams();
    request->trusted_params->has_user_activation = has_user_activation;
  }

  std::string mode;
  if (opts.Get("mode", &mode) && !mode.empty()) {
    if (mode == "navigate") {
      request->mode = network::mojom::RequestMode::kNavigate;
    } else if (mode == "cors") {
      request->mode = network::mojom::RequestMode::kCors;
    } else if (mode == "no-cors") {
      request->mode = network::mojom::RequestMode::kNoCors;
    } else if (mode == "same-origin") {
      request->mode = network::mojom::RequestMode::kSameOrigin;
    }
  }

  std::string destination;
  if (opts.Get("destination", &destination) && !destination.empty()) {
    if (destination == "empty") {
      request->destination = network::mojom::RequestDestination::kEmpty;
    } else if (destination == "audio") {
      request->destination = network::mojom::RequestDestination::kAudio;
    } else if (destination == "audioworklet") {
      request->destination = network::mojom::RequestDestination::kAudioWorklet;
    } else if (destination == "document") {
      request->destination = network::mojom::RequestDestination::kDocument;
    } else if (destination == "embed") {
      request->destination = network::mojom::RequestDestination::kEmbed;
    } else if (destination == "font") {
      request->destination = network::mojom::RequestDestination::kFont;
    } else if (destination == "frame") {
      request->destination = network::mojom::RequestDestination::kFrame;
    } else if (destination == "iframe") {
      request->destination = network::mojom::RequestDestination::kIframe;
    } else if (destination == "image") {
      request->destination = network::mojom::RequestDestination::kImage;
    } else if (destination == "manifest") {
      request->destination = network::mojom::RequestDestination::kManifest;
    } else if (destination == "object") {
      request->destination = network::mojom::RequestDestination::kObject;
    } else if (destination == "paintworklet") {
      request->destination = network::mojom::RequestDestination::kPaintWorklet;
    } else if (destination == "report") {
      request->destination = network::mojom::RequestDestination::kReport;
    } else if (destination == "script") {
      request->destination = network::mojom::RequestDestination::kScript;
    } else if (destination == "serviceworker") {
      request->destination = network::mojom::RequestDestination::kServiceWorker;
    } else if (destination == "style") {
      request->destination = network::mojom::RequestDestination::kStyle;
    } else if (destination == "track") {
      request->destination = network::mojom::RequestDestination::kTrack;
    } else if (destination == "video") {
      request->destination = network::mojom::RequestDestination::kVideo;
    } else if (destination == "worker") {
      request->destination = network::mojom::RequestDestination::kWorker;
    } else if (destination == "xslt") {
      request->destination = network::mojom::RequestDestination::kXslt;
    }
  }

  bool credentials_specified =
      opts.Get("credentials", &request->credentials_mode);
  std::vector<std::pair<std::string, std::string>> extra_headers;
  if (opts.Get("extraHeaders", &extra_headers)) {
    for (const auto& it : extra_headers) {
      if (!net::HttpUtil::IsValidHeaderName(it.first) ||
          !net::HttpUtil::IsValidHeaderValue(it.second)) {
        args->ThrowTypeError("Invalid header name or value");
        return gin::Handle<SimpleURLLoaderWrapper>();
      }
      request->headers.SetHeader(it.first, it.second);
    }
  }

  bool use_session_cookies = false;
  opts.Get("useSessionCookies", &use_session_cookies);
  int options = 0;
  if (!credentials_specified && !use_session_cookies) {
    // 这是默认情况，也是凭据未设置时的情况。
    // Specified and useSessionCoookies为false。Credentials_mode将为。
    // K包含，但Cookie将被阻止。
    request->credentials_mode = network::mojom::CredentialsMode::kInclude;
    options |= network::mojom::kURLLoadOptionBlockAllCookies;
  }

  v8::Local<v8::Value> body;
  v8::Local<v8::Value> chunk_pipe_getter;
  if (opts.Get("body", &body)) {
    if (body->IsArrayBufferView()) {
      auto buffer_body = body.As<v8::ArrayBufferView>();
      auto backing_store = buffer_body->Buffer()->GetBackingStore();
      request->request_body = network::ResourceRequestBody::CreateFromBytes(
          static_cast<char*>(backing_store->Data()) + buffer_body->ByteOffset(),
          buffer_body->ByteLength());
    } else if (body->IsFunction()) {
      auto body_func = body.As<v8::Function>();

      mojo::PendingRemote<network::mojom::ChunkedDataPipeGetter>
          data_pipe_getter;
      chunk_pipe_getter = JSChunkedDataPipeGetter::Create(
                              args->isolate(), body_func,
                              data_pipe_getter.InitWithNewPipeAndPassReceiver())
                              .ToV8();
      request->request_body =
          base::MakeRefCounted<network::ResourceRequestBody>();
      request->request_body->SetToChunkedDataPipe(
          std::move(data_pipe_getter),
          network::ResourceRequestBody::ReadOnlyOnce(false));
    }
  }

  std::string partition;
  gin::Handle<Session> session;
  if (!opts.Get("session", &session)) {
    if (opts.Get("partition", &partition))
      session = Session::FromPartition(args->isolate(), partition);
    else  // 默认会话。
      session = Session::FromPartition(args->isolate(), "");
  }

  auto url_loader_factory = session->browser_context()->GetURLLoaderFactory();

  auto ret = gin::CreateHandle(
      args->isolate(),
      new SimpleURLLoaderWrapper(std::move(request), url_loader_factory.get(),
                                 options));
  ret->Pin();
  if (!chunk_pipe_getter.IsEmpty()) {
    ret->PinBodyGetter(chunk_pipe_getter);
  }
  return ret;
}

void SimpleURLLoaderWrapper::OnDataReceived(base::StringPiece string_piece,
                                            base::OnceClosure resume) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto array_buffer = v8::ArrayBuffer::New(isolate, string_piece.size());
  auto backing_store = array_buffer->GetBackingStore();
  memcpy(backing_store->Data(), string_piece.data(), string_piece.size());
  Emit("data", array_buffer,
       base::AdaptCallbackForRepeating(std::move(resume)));
}

void SimpleURLLoaderWrapper::OnComplete(bool success) {
  if (success) {
    Emit("complete");
  } else {
    Emit("error", net::ErrorToString(loader_->NetError()));
  }
  loader_.reset();
  pinned_wrapper_.Reset();
  pinned_chunk_pipe_getter_.Reset();
}

void SimpleURLLoaderWrapper::OnRetry(base::OnceClosure start_retry) {}

void SimpleURLLoaderWrapper::OnResponseStarted(
    const GURL& final_url,
    const network::mojom::URLResponseHead& response_head) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  gin::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("statusCode", response_head.headers->response_code());
  dict.Set("statusMessage", response_head.headers->GetStatusText());
  dict.Set("httpVersion", response_head.headers->GetHttpVersion());
  // 请注意，|RESPONSE_Head.Headers|由Chromium过滤，不应。
  // 在这里使用。
  DCHECK(!response_head.raw_response_headers.empty());
  dict.Set("rawHeaders", response_head.raw_response_headers);
  Emit("response-started", final_url, dict);
}

void SimpleURLLoaderWrapper::OnRedirect(
    const net::RedirectInfo& redirect_info,
    const network::mojom::URLResponseHead& response_head,
    std::vector<std::string>* removed_headers) {
  Emit("redirect", redirect_info, response_head.headers.get());
}

void SimpleURLLoaderWrapper::OnUploadProgress(uint64_t position,
                                              uint64_t total) {
  Emit("upload-progress", position, total);
}

void SimpleURLLoaderWrapper::OnDownloadProgress(uint64_t current) {
  Emit("download-progress", current);
}

// 静电。
gin::ObjectTemplateBuilder SimpleURLLoaderWrapper::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<
             SimpleURLLoaderWrapper>::GetObjectTemplateBuilder(isolate)
      .SetMethod("cancel", &SimpleURLLoaderWrapper::Cancel);
}

const char* SimpleURLLoaderWrapper::GetTypeName() {
  return "SimpleURLLoaderWrapper";
}

}  // 命名空间API。

}  // 命名空间电子
