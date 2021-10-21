// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_CALLER_H_
#define SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_CALLER_H_

#include <utility>
#include <vector>

#include "gin/converter.h"
#include "gin/wrappable.h"

namespace gin_helper {

namespace internal {

using ValueVector = std::vector<v8::Local<v8::Value>>;

v8::Local<v8::Value> CallMethodWithArgs(v8::Isolate* isolate,
                                        v8::Local<v8::Object> obj,
                                        const char* method,
                                        ValueVector* args);

}  // 命名空间内部。

// Obj.emit.Apply(obj，name，args...)；
// 调用方负责分配HandleScope。
template <typename StringType>
v8::Local<v8::Value> EmitEvent(v8::Isolate* isolate,
                               v8::Local<v8::Object> obj,
                               const StringType& name,
                               const internal::ValueVector& args) {
  internal::ValueVector concatenated_args = {gin::StringToV8(isolate, name)};
  concatenated_args.reserve(1 + args.size());
  concatenated_args.insert(concatenated_args.end(), args.begin(), args.end());
  return internal::CallMethodWithArgs(isolate, obj, "emit", &concatenated_args);
}

// Obj.emit(名称，参数...)；
// 调用方负责分配HandleScope。
template <typename StringType, typename... Args>
v8::Local<v8::Value> EmitEvent(v8::Isolate* isolate,
                               v8::Local<v8::Object> obj,
                               const StringType& name,
                               Args&&... args) {
  internal::ValueVector converted_args = {
      gin::StringToV8(isolate, name),
      gin::ConvertToV8(isolate, std::forward<Args>(args))...,
  };
  return internal::CallMethodWithArgs(isolate, obj, "emit", &converted_args);
}

// Obj.customemit(args...)。
template <typename... Args>
v8::Local<v8::Value> CustomEmit(v8::Isolate* isolate,
                                v8::Local<v8::Object> object,
                                const char* custom_emit,
                                Args&&... args) {
  internal::ValueVector converted_args = {
      gin::ConvertToV8(isolate, std::forward<Args>(args))...,
  };
  return internal::CallMethodWithArgs(isolate, object, custom_emit,
                                      &converted_args);
}

template <typename T, typename... Args>
v8::Local<v8::Value> CallMethod(v8::Isolate* isolate,
                                gin::Wrappable<T>* object,
                                const char* method_name,
                                Args&&... args) {
  v8::EscapableHandleScope scope(isolate);
  v8::Local<v8::Object> v8_object;
  if (object->GetWrapper(isolate).ToLocal(&v8_object))
    return scope.Escape(CustomEmit(isolate, v8_object, method_name,
                                   std::forward<Args>(args)...));
  else
    return v8::Local<v8::Value>();
}

template <typename T, typename... Args>
v8::Local<v8::Value> CallMethod(gin::Wrappable<T>* object,
                                const char* method_name,
                                Args&&... args) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  return CallMethod(isolate, object, method_name, std::forward<Args>(args)...);
}

}  // 命名空间gin_helper。

#endif  // SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_CALLER_H_
