// 版权所有2020 Slake Technologies，Inc.。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在LICENSE.Cr文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_EXTENSIONS_H_
#define SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_EXTENSIONS_H_

#include <utility>

#include "gin/function_template.h"
#include "shell/common/gin_helper/error_thrower.h"

// 这扩展了//gin/function_template.h中“Special”的功能。
// GIN绑定方法的参数。
// 它相当于function_template.h，其中包含以下方法。
// 在gin_helper名称空间中。
namespace gin {

// 支持将ABSL：：OPTIONAL作为参数。
template <typename T>
bool GetNextArgument(Arguments* args,
                     const InvokerOptions& invoker_options,
                     bool is_first,
                     absl::optional<T>* result) {
  T converted;
  // 使用gin：：Arguments：：GetNext，它始终前进|Next|Counter。
  if (args->GetNext(&converted))
    result->emplace(std::move(converted));
  return true;
}

inline bool GetNextArgument(Arguments* args,
                            const InvokerOptions& invoker_options,
                            bool is_first,
                            gin_helper::ErrorThrower* result) {
  *result = gin_helper::ErrorThrower(args->isolate());
  return true;
}

// 类似gin：：CreateFunctionTemplate，但不删除模板的。
// 原型。
template <typename Sig>
v8::Local<v8::FunctionTemplate> CreateConstructorFunctionTemplate(
    v8::Isolate* isolate,
    base::RepeatingCallback<Sig> callback,
    InvokerOptions invoker_options = {}) {
  typedef internal::CallbackHolder<Sig> HolderT;
  HolderT* holder =
      new HolderT(isolate, std::move(callback), std::move(invoker_options));

  v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(
      isolate, &internal::Dispatcher<Sig>::DispatchToCallback,
      ConvertToV8<v8::Local<v8::External>>(isolate,
                                           holder->GetHandle(isolate)));
  return tmpl;
}

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_EXTENSIONS_H_
