// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_H_

#include "gin/converter.h"

namespace base {
class DictionaryValue;
class ListValue;
class Value;
}  // 命名空间库。

namespace gin {

template <>
struct Converter<base::DictionaryValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::DictionaryValue* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::DictionaryValue& val);
};

template <>
struct Converter<base::Value> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Value* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Value& val);
};

template <>
struct Converter<base::ListValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::ListValue* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::ListValue& val);
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_H_
