// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_

#include <string>

#include "gin/converter.h"

namespace extensions {
class Extension;
}

namespace gin {

template <>
struct Converter<const extensions::Extension*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const extensions::Extension* val);
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_CONVERTERS_EXTENSION_CONVERTER_H_
