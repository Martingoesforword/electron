// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_API_ELECTRON_API_CONTEXT_BRIDGE_H_
#define SHELL_RENDERER_API_ELECTRON_API_CONTEXT_BRIDGE_H_

#include "shell/renderer/api/context_bridge/object_cache.h"
#include "v8/include/v8.h"

namespace gin_helper {
class Arguments;
}

namespace electron {

namespace api {

void ProxyFunctionWrapper(const v8::FunctionCallbackInfo<v8::Value>& info);

// 上下文桥应该在其中创建它即将抛出的异常。
enum class BridgeErrorTarget {
  // 源/调用上下文。这是默认设置，99%的情况下都是正确的，
  // 请求转换的调用者/上下文将收到错误。
  // 因此，应在该上下文中犯下错误。
  kSource,
  // 目标/目标上下文。此选项仅在以下情况下使用：源。
  // 将不会捕获它正在传递的值所导致的错误。
  // 桥。这**仅**在从函数返回值时**才会发生。
  // 我们在方法终止和执行后转换返回值。
  // 已返回给调用方。在这种情况下，错误将是。
  // 可在“Destination”上下文中捕获，因此我们创建了错误。
  // 那里。
  kDestination
};

v8::MaybeLocal<v8::Value> PassValueToOtherContext(
    v8::Local<v8::Context> source_context,
    v8::Local<v8::Context> destination_context,
    v8::Local<v8::Value> value,
    context_bridge::ObjectCache* object_cache,
    bool support_dynamic_properties,
    int recursion_depth,
    BridgeErrorTarget error_target = BridgeErrorTarget::kSource);

v8::MaybeLocal<v8::Object> CreateProxyForAPI(
    const v8::Local<v8::Object>& api_object,
    const v8::Local<v8::Context>& source_context,
    const v8::Local<v8::Context>& destination_context,
    context_bridge::ObjectCache* object_cache,
    bool support_dynamic_properties,
    int recursion_depth);

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_RENDERER_API_ELECTRON_API_CONTEXT_BRIDGE_H_
