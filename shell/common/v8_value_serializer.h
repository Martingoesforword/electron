// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_V8_VALUE_SERIALIZER_H_
#define SHELL_COMMON_V8_VALUE_SERIALIZER_H_

#include "base/containers/span.h"

namespace v8 {
class Isolate;
template <class T>
class Local;
class Value;
}  // 命名空间V8。

namespace blink {
struct CloneableMessage;
}

namespace electron {

bool SerializeV8Value(v8::Isolate* isolate,
                      v8::Local<v8::Value> value,
                      blink::CloneableMessage* out);
v8::Local<v8::Value> DeserializeV8Value(v8::Isolate* isolate,
                                        const blink::CloneableMessage& in);
v8::Local<v8::Value> DeserializeV8Value(v8::Isolate* isolate,
                                        base::span<const uint8_t> data);

}  // 命名空间电子。

#endif  // SHELL_COMMON_V8_VALUE_序列化程序_H_
