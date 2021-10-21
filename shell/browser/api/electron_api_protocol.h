// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_PROTOCOL_H_
#define SHELL_BROWSER_API_ELECTRON_API_PROTOCOL_H_

#include <string>
#include <vector>

#include "content/public/browser/content_browser_client.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/net/electron_url_loader_factory.h"

namespace electron {

class ElectronBrowserContext;
class ProtocolRegistry;

namespace api {

std::vector<std::string> GetStandardSchemes();

void AddServiceWorkerScheme(const std::string& scheme);

void RegisterSchemesAsPrivileged(gin_helper::ErrorThrower thrower,
                                 v8::Local<v8::Value> val);

// 可能的错误。
enum class ProtocolError {
  kOK,  // 无错误。
  kRegistered,
  kNotRegistered,
  kIntercepted,
  kNotIntercepted,
};

// 基于网络服务的协议实现。
class Protocol : public gin::Wrappable<Protocol> {
 public:
  static gin::Handle<Protocol> Create(v8::Isolate* isolate,
                                      ElectronBrowserContext* browser_context);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 private:
  Protocol(v8::Isolate* isolate, ProtocolRegistry* protocol_registry);
  ~Protocol() override;

  // 回调类型。
  using CompletionCallback =
      base::RepeatingCallback<void(v8::Local<v8::Value>)>;

  // JS接口。
  ProtocolError RegisterProtocol(ProtocolType type,
                                 const std::string& scheme,
                                 const ProtocolHandler& handler);
  bool UnregisterProtocol(const std::string& scheme, gin::Arguments* args);
  bool IsProtocolRegistered(const std::string& scheme);

  ProtocolError InterceptProtocol(ProtocolType type,
                                  const std::string& scheme,
                                  const ProtocolHandler& handler);
  bool UninterceptProtocol(const std::string& scheme, gin::Arguments* args);
  bool IsProtocolIntercepted(const std::string& scheme);

  // IsProtocolRegisted的旧异步版本。
  v8::Local<v8::Promise> IsProtocolHandled(const std::string& scheme,
                                           gin::Arguments* args);

  // 将旧的注册API转换为新的RegisterProtocol API的帮助器。
  template <ProtocolType type>
  bool RegisterProtocolFor(const std::string& scheme,
                           const ProtocolHandler& handler,
                           gin::Arguments* args) {
    auto result = RegisterProtocol(type, scheme, handler);
    HandleOptionalCallback(args, result);
    return result == ProtocolError::kOK;
  }
  template <ProtocolType type>
  bool InterceptProtocolFor(const std::string& scheme,
                            const ProtocolHandler& handler,
                            gin::Arguments* args) {
    auto result = InterceptProtocol(type, scheme, handler);
    HandleOptionalCallback(args, result);
    return result == ProtocolError::kOK;
  }

  // 兼容旧接口，支持可选回调。
  void HandleOptionalCallback(gin::Arguments* args, ProtocolError error);

  // 弱指针；ProtocolRegistry的生存期保证为。
  // 比此JS接口的生命周期更长。
  ProtocolRegistry* protocol_registry_;
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Electronics_API_PROTOCOL_H_
