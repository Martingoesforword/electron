// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_TIME_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_TIME_CONVERTER_H_

#include "gin/converter.h"

namespace base {
class Time;
}

namespace gin {

template <>
struct Converter<base::Time> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const base::Time& val);
};

}  // 命名空间杜松子酒。

#endif  // Shell_common_gin_Converters_Time_Converter_H_
