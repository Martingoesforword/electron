// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_WEB_REQUEST_H_
#define SHELL_BROWSER_API_ELECTRON_API_WEB_REQUEST_H_

#include <map>
#include <set>

#include "base/values.h"
#include "extensions/common/url_pattern.h"
#include "gin/arguments.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/net/web_request_api_interface.h"

namespace content {
class BrowserContext;
}

namespace electron {

namespace api {

class WebRequest : public gin::Wrappable<WebRequest>, public WebRequestAPI {
 public:
  static gin::WrapperInfo kWrapperInfo;

  // 返回附加到|BROWSER_CONTEXT|的WebRequest对象，如果存在。
  // 没有人。
  // 请注意，WebRequest对象的生存期由会话管理，而不是由会话管理。
  // 呼叫者的名字。
  static gin::Handle<WebRequest> FromOrCreate(
      v8::Isolate* isolate,
      content::BrowserContext* browser_context);

  // 返回一个新的WebRequest对象，它应该只由Session调用。
  static gin::Handle<WebRequest> Create(
      v8::Isolate* isolate,
      content::BrowserContext* browser_context);

  // 查找附加到|BROWSER_CONTEXT|的WebRequest对象。
  static gin::Handle<WebRequest> From(v8::Isolate* isolate,
                                      content::BrowserContext* browser_context);

  // 杜松子酒：：可包装的：
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // WebRequestAPI：
  bool HasListener() const override;
  int OnBeforeRequest(extensions::WebRequestInfo* info,
                      const network::ResourceRequest& request,
                      net::CompletionOnceCallback callback,
                      GURL* new_url) override;
  int OnBeforeSendHeaders(extensions::WebRequestInfo* info,
                          const network::ResourceRequest& request,
                          BeforeSendHeadersCallback callback,
                          net::HttpRequestHeaders* headers) override;
  int OnHeadersReceived(
      extensions::WebRequestInfo* info,
      const network::ResourceRequest& request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;
  void OnSendHeaders(extensions::WebRequestInfo* info,
                     const network::ResourceRequest& request,
                     const net::HttpRequestHeaders& headers) override;
  void OnBeforeRedirect(extensions::WebRequestInfo* info,
                        const network::ResourceRequest& request,
                        const GURL& new_location) override;
  void OnResponseStarted(extensions::WebRequestInfo* info,
                         const network::ResourceRequest& request) override;
  void OnErrorOccurred(extensions::WebRequestInfo* info,
                       const network::ResourceRequest& request,
                       int net_error) override;
  void OnCompleted(extensions::WebRequestInfo* info,
                   const network::ResourceRequest& request,
                   int net_error) override;
  void OnRequestWillBeDestroyed(extensions::WebRequestInfo* info) override;

 private:
  WebRequest(v8::Isolate* isolate, content::BrowserContext* browser_context);
  ~WebRequest() override;

  enum class SimpleEvent {
    kOnSendHeaders,
    kOnBeforeRedirect,
    kOnResponseStarted,
    kOnCompleted,
    kOnErrorOccurred,
  };
  enum class ResponseEvent {
    kOnBeforeRequest,
    kOnBeforeSendHeaders,
    kOnHeadersReceived,
  };

  using SimpleListener = base::RepeatingCallback<void(v8::Local<v8::Value>)>;
  using ResponseCallback = base::OnceCallback<void(v8::Local<v8::Value>)>;
  using ResponseListener =
      base::RepeatingCallback<void(v8::Local<v8::Value>, ResponseCallback)>;

  template <SimpleEvent event>
  void SetSimpleListener(gin::Arguments* args);
  template <ResponseEvent event>
  void SetResponseListener(gin::Arguments* args);
  template <typename Listener, typename Listeners, typename Event>
  void SetListener(Event event, Listeners* listeners, gin::Arguments* args);

  template <typename... Args>
  void HandleSimpleEvent(SimpleEvent event,
                         extensions::WebRequestInfo* info,
                         Args... args);
  template <typename Out, typename... Args>
  int HandleResponseEvent(ResponseEvent event,
                          extensions::WebRequestInfo* info,
                          net::CompletionOnceCallback callback,
                          Out out,
                          Args... args);

  template <typename T>
  void OnListenerResult(uint64_t id, T out, v8::Local<v8::Value> response);

  struct SimpleListenerInfo {
    std::set<URLPattern> url_patterns;
    SimpleListener listener;

    SimpleListenerInfo(std::set<URLPattern>, SimpleListener);
    SimpleListenerInfo();
    ~SimpleListenerInfo();
  };

  struct ResponseListenerInfo {
    std::set<URLPattern> url_patterns;
    ResponseListener listener;

    ResponseListenerInfo(std::set<URLPattern>, ResponseListener);
    ResponseListenerInfo();
    ~ResponseListenerInfo();
  };

  std::map<SimpleEvent, SimpleListenerInfo> simple_listeners_;
  std::map<ResponseEvent, ResponseListenerInfo> response_listeners_;
  std::map<uint64_t, net::CompletionOnceCallback> callbacks_;

  // 弱裁判，它管理着我们。
  content::BrowserContext* browser_context_;
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Electronics_API_Web_Request_H_
