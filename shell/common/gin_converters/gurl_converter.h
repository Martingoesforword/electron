// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_GURL_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_GURL_CONVERTER_H_

#include <string>

#include "gin/converter.h"
#include "url/gurl.h"

namespace gin {

template <>
struct Converter<GURL> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const GURL& val) {
    return ConvertToV8(isolate, val.spec());
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     GURL* out) {
    std::string url;
    if (Converter<std::string>::FromV8(isolate, val, &url)) {
      *out = GURL(url);
      return true;
    } else {
      return false;
    }
  }
};

}  // 命名空间杜松子酒。

#endif  // Shell_COMMON_GIN_CONVERTERS_Gurl_Converter_H_
