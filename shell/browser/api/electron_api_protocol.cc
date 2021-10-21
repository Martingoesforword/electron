// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_protocol.h"

#include <vector>

#include "base/command_line.h"
#include "base/stl_util.h"
#include "content/common/url_schemes.h"
#include "content/public/browser/child_process_security_policy.h"
#include "gin/object_template_builder.h"
#include "shell/browser/browser.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/protocol_registry.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/common/process_util.h"
#include "url/url_util.h"

namespace {

// 已注册的自定义标准方案列表。
std::vector<std::string> g_standard_schemes;

// 已注册的自定义流方案列表。
std::vector<std::string> g_streaming_schemes;

struct SchemeOptions {
  bool standard = false;
  bool secure = false;
  bool bypassCSP = false;
  bool allowServiceWorkers = false;
  bool supportFetchAPI = false;
  bool corsEnabled = false;
  bool stream = false;
};

struct CustomScheme {
  std::string scheme;
  SchemeOptions options;
};

}  // 命名空间。

namespace gin {

template <>
struct Converter<CustomScheme> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     CustomScheme* out) {
    gin::Dictionary dict(isolate);
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    if (!dict.Get("scheme", &(out->scheme)))
      return false;
    gin::Dictionary opt(isolate);
    // 选项是可选的。使用在SchemeOptions中指定的默认值。
    if (dict.Get("privileges", &opt)) {
      opt.Get("standard", &(out->options.standard));
      opt.Get("secure", &(out->options.secure));
      opt.Get("bypassCSP", &(out->options.bypassCSP));
      opt.Get("allowServiceWorkers", &(out->options.allowServiceWorkers));
      opt.Get("supportFetchAPI", &(out->options.supportFetchAPI));
      opt.Get("corsEnabled", &(out->options.corsEnabled));
      opt.Get("stream", &(out->options.stream));
    }
    return true;
  }
};

}  // 命名空间杜松子酒。

namespace electron {
namespace api {

gin::WrapperInfo Protocol::kWrapperInfo = {gin::kEmbedderNativeGin};

std::vector<std::string> GetStandardSchemes() {
  return g_standard_schemes;
}

void AddServiceWorkerScheme(const std::string& scheme) {
  // 没有用于添加服务工作者方案的API，但有一个用于添加服务工作者方案的API。
  // 返回对方案向量的常量引用。
  // 如果将来API改变为返回副本而不是引用，
  // 编译将失败，我们应该在那时添加一个补丁。
  auto& mutable_schemes =
      const_cast<std::vector<std::string>&>(content::GetServiceWorkerSchemes());
  mutable_schemes.push_back(scheme);
}

void RegisterSchemesAsPrivileged(gin_helper::ErrorThrower thrower,
                                 v8::Local<v8::Value> val) {
  std::vector<CustomScheme> custom_schemes;
  if (!gin::ConvertFromV8(thrower.isolate(), val, &custom_schemes)) {
    thrower.ThrowError("Argument must be an array of custom schemes.");
    return;
  }

  std::vector<std::string> secure_schemes, cspbypassing_schemes, fetch_schemes,
      service_worker_schemes, cors_schemes;
  for (const auto& custom_scheme : custom_schemes) {
    // 将方案注册到特权列表(https、wss、data、Chrome-Extension)。
    if (custom_scheme.options.standard) {
      auto* policy = content::ChildProcessSecurityPolicy::GetInstance();
      url::AddStandardScheme(custom_scheme.scheme.c_str(),
                             url::SCHEME_WITH_HOST);
      g_standard_schemes.push_back(custom_scheme.scheme);
      policy->RegisterWebSafeScheme(custom_scheme.scheme);
    }
    if (custom_scheme.options.secure) {
      secure_schemes.push_back(custom_scheme.scheme);
      url::AddSecureScheme(custom_scheme.scheme.c_str());
    }
    if (custom_scheme.options.bypassCSP) {
      cspbypassing_schemes.push_back(custom_scheme.scheme);
      url::AddCSPBypassingScheme(custom_scheme.scheme.c_str());
    }
    if (custom_scheme.options.corsEnabled) {
      cors_schemes.push_back(custom_scheme.scheme);
      url::AddCorsEnabledScheme(custom_scheme.scheme.c_str());
    }
    if (custom_scheme.options.supportFetchAPI) {
      fetch_schemes.push_back(custom_scheme.scheme);
    }
    if (custom_scheme.options.allowServiceWorkers) {
      service_worker_schemes.push_back(custom_scheme.scheme);
      AddServiceWorkerScheme(custom_scheme.scheme);
    }
    if (custom_scheme.options.stream) {
      g_streaming_schemes.push_back(custom_scheme.scheme);
    }
  }

  const auto AppendSchemesToCmdLine = [](const char* switch_name,
                                         std::vector<std::string> schemes) {
    // 将方案添加到命令行开关，以便子进程也可以。
    // 注册他们。
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switch_name, base::JoinString(schemes, ","));
  };

  AppendSchemesToCmdLine(electron::switches::kSecureSchemes, secure_schemes);
  AppendSchemesToCmdLine(electron::switches::kBypassCSPSchemes,
                         cspbypassing_schemes);
  AppendSchemesToCmdLine(electron::switches::kCORSSchemes, cors_schemes);
  AppendSchemesToCmdLine(electron::switches::kFetchSchemes, fetch_schemes);
  AppendSchemesToCmdLine(electron::switches::kServiceWorkerSchemes,
                         service_worker_schemes);
  AppendSchemesToCmdLine(electron::switches::kStandardSchemes,
                         g_standard_schemes);
  AppendSchemesToCmdLine(electron::switches::kStreamingSchemes,
                         g_streaming_schemes);
}

namespace {

const char* const kBuiltinSchemes[] = {
    "about", "file", "http", "https", "data", "filesystem",
};

// 将错误代码转换为字符串。
std::string ErrorCodeToString(ProtocolError error) {
  switch (error) {
    case ProtocolError::kRegistered:
      return "The scheme has been registered";
    case ProtocolError::kNotRegistered:
      return "The scheme has not been registered";
    case ProtocolError::kIntercepted:
      return "The scheme has been intercepted";
    case ProtocolError::kNotIntercepted:
      return "The scheme has not been intercepted";
    default:
      return "Unexpected error";
  }
}

}  // 命名空间。

Protocol::Protocol(v8::Isolate* isolate, ProtocolRegistry* protocol_registry)
    : protocol_registry_(protocol_registry) {}

Protocol::~Protocol() = default;

ProtocolError Protocol::RegisterProtocol(ProtocolType type,
                                         const std::string& scheme,
                                         const ProtocolHandler& handler) {
  bool added = protocol_registry_->RegisterProtocol(type, scheme, handler);
  return added ? ProtocolError::kOK : ProtocolError::kRegistered;
}

bool Protocol::UnregisterProtocol(const std::string& scheme,
                                  gin::Arguments* args) {
  bool removed = protocol_registry_->UnregisterProtocol(scheme);
  HandleOptionalCallback(
      args, removed ? ProtocolError::kOK : ProtocolError::kNotRegistered);
  return removed;
}

bool Protocol::IsProtocolRegistered(const std::string& scheme) {
  return protocol_registry_->IsProtocolRegistered(scheme);
}

ProtocolError Protocol::InterceptProtocol(ProtocolType type,
                                          const std::string& scheme,
                                          const ProtocolHandler& handler) {
  bool added = protocol_registry_->InterceptProtocol(type, scheme, handler);
  return added ? ProtocolError::kOK : ProtocolError::kIntercepted;
}

bool Protocol::UninterceptProtocol(const std::string& scheme,
                                   gin::Arguments* args) {
  bool removed = protocol_registry_->UninterceptProtocol(scheme);
  HandleOptionalCallback(
      args, removed ? ProtocolError::kOK : ProtocolError::kNotIntercepted);
  return removed;
}

bool Protocol::IsProtocolIntercepted(const std::string& scheme) {
  return protocol_registry_->IsProtocolIntercepted(scheme);
}

v8::Local<v8::Promise> Protocol::IsProtocolHandled(const std::string& scheme,
                                                   gin::Arguments* args) {
  node::Environment* env = node::Environment::GetCurrent(args->isolate());
  EmitWarning(env,
              "The protocol.isProtocolHandled API is deprecated, use "
              "protocol.isProtocolRegistered or protocol.isProtocolIntercepted "
              "instead.",
              "ProtocolDeprecateIsProtocolHandled");
  return gin_helper::Promise<bool>::ResolvedPromise(
      args->isolate(),
      IsProtocolRegistered(scheme) || IsProtocolIntercepted(scheme) ||
          // 对于构建，|isProtocolHandled|应返回TRUE。
          // 方案，但是使用NetworkService不可能。
          // 知道哪些计划已注册，直到真正的网络。
          // 请求已发送。
          // 因此我们必须针对硬编码的内置方案进行测试。
          // 列表使其与旧代码一起工作。我们应该反对。
          // 此接口与新的|isProtocolRegisted|接口配合使用。
          base::Contains(kBuiltinSchemes, scheme));
}

void Protocol::HandleOptionalCallback(gin::Arguments* args,
                                      ProtocolError error) {
  base::RepeatingCallback<void(v8::Local<v8::Value>)> callback;
  if (args->GetNext(&callback)) {
    node::Environment* env = node::Environment::GetCurrent(args->isolate());
    EmitWarning(
        env,
        "The callback argument of protocol module APIs is no longer needed.",
        "ProtocolDeprecateCallback");
    if (error == ProtocolError::kOK)
      callback.Run(v8::Null(args->isolate()));
    else
      callback.Run(v8::Exception::Error(
          gin::StringToV8(args->isolate(), ErrorCodeToString(error))));
  }
}

// 静电。
gin::Handle<Protocol> Protocol::Create(
    v8::Isolate* isolate,
    ElectronBrowserContext* browser_context) {
  return gin::CreateHandle(
      isolate, new Protocol(isolate, browser_context->protocol_registry()));
}

gin::ObjectTemplateBuilder Protocol::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<Protocol>::GetObjectTemplateBuilder(isolate)
      .SetMethod("registerStringProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kString>)
      .SetMethod("registerBufferProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kBuffer>)
      .SetMethod("registerFileProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kFile>)
      .SetMethod("registerHttpProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kHttp>)
      .SetMethod("registerStreamProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kStream>)
      .SetMethod("registerProtocol",
                 &Protocol::RegisterProtocolFor<ProtocolType::kFree>)
      .SetMethod("unregisterProtocol", &Protocol::UnregisterProtocol)
      .SetMethod("isProtocolRegistered", &Protocol::IsProtocolRegistered)
      .SetMethod("isProtocolHandled", &Protocol::IsProtocolHandled)
      .SetMethod("interceptStringProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kString>)
      .SetMethod("interceptBufferProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kBuffer>)
      .SetMethod("interceptFileProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kFile>)
      .SetMethod("interceptHttpProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kHttp>)
      .SetMethod("interceptStreamProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kStream>)
      .SetMethod("interceptProtocol",
                 &Protocol::InterceptProtocolFor<ProtocolType::kFree>)
      .SetMethod("uninterceptProtocol", &Protocol::UninterceptProtocol)
      .SetMethod("isProtocolIntercepted", &Protocol::IsProtocolIntercepted);
}

const char* Protocol::GetTypeName() {
  return "Protocol";
}

}  // 命名空间API。
}  // 命名空间电子。

namespace {

void RegisterSchemesAsPrivileged(gin_helper::ErrorThrower thrower,
                                 v8::Local<v8::Value> val) {
  if (electron::Browser::Get()->is_ready()) {
    thrower.ThrowError(
        "protocol.registerSchemesAsPrivileged should be called before "
        "app is ready");
    return;
  }

  electron::api::RegisterSchemesAsPrivileged(thrower, val);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("registerSchemesAsPrivileged", &RegisterSchemesAsPrivileged);
  dict.SetMethod("getStandardSchemes", &electron::api::GetStandardSchemes);
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_protocol, Initialize)
