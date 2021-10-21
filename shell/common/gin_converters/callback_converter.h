// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_H_

#include <utility>

#include "base/callback_helpers.h"
#include "shell/common/gin_helper/callback.h"

namespace gin {

template <typename Sig>
struct Converter<base::RepeatingCallback<Sig>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::RepeatingCallback<Sig>& val) {
    // 我们在这里不使用CreateFunctionTemplate，因为它创建了一个新的。
    // 每次都使用FunctionTemplate，它由V8缓存，会导致泄漏。
    auto translater =
        base::BindRepeating(&gin_helper::NativeFunctionInvoker<Sig>::Go, val);
    // 为避免内存泄漏，我们确保只能调用回调。
    // 就这一次。
    return gin_helper::CreateFunctionFromTranslater(isolate, translater, true);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::RepeatingCallback<Sig>* out) {
    if (!val->IsFunction())
      return false;

    *out = base::BindRepeating(&gin_helper::V8FunctionInvoker<Sig>::Go, isolate,
                               gin_helper::SafeV8Function(isolate, val));
    return true;
  }
};

template <typename Sig>
struct Converter<base::OnceCallback<Sig>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   base::OnceCallback<Sig> in) {
    return gin::ConvertToV8(isolate,
                            base::AdaptCallbackForRepeating(std::move(in)));
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::OnceCallback<Sig>* out) {
    if (!val->IsFunction())
      return false;
    *out = base::BindOnce(&gin_helper::V8FunctionInvoker<Sig>::Go, isolate,
                          gin_helper::SafeV8Function(isolate, val));
    return true;
  }
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_H_
