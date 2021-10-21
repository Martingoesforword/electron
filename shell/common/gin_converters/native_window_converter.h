// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_NATIVE_WINDOW_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_NATIVE_WINDOW_CONVERTER_H_

#include "gin/converter.h"
#include "shell/browser/api/electron_api_base_window.h"

namespace gin {

template <>
struct Converter<electron::NativeWindow*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::NativeWindow** out) {
    // NULL将被转移到NULL。
    if (val->IsNull()) {
      *out = NULL;
      return true;
    }

    electron::api::BaseWindow* window;
    if (!gin::Converter<electron::api::BaseWindow*>::FromV8(isolate, val,
                                                            &window))
      return false;
    *out = window->window();
    return true;
  }
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_CONVERTERS_NATIVE_WINDOW_CONVERTER_H_
