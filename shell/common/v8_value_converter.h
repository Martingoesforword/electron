// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_V8_VALUE_CONVERTER_H_
#define SHELL_COMMON_V8_VALUE_CONVERTER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "v8/include/v8.h"

namespace base {
class DictionaryValue;
class ListValue;
class Value;
}  // 命名空间库。

namespace electron {

class V8ValueConverter {
 public:
  V8ValueConverter();

  void SetRegExpAllowed(bool val);
  void SetFunctionAllowed(bool val);
  void SetStripNullFromObjects(bool val);
  v8::Local<v8::Value> ToV8Value(const base::Value* value,
                                 v8::Local<v8::Context> context) const;
  std::unique_ptr<base::Value> FromV8Value(
      v8::Local<v8::Value> value,
      v8::Local<v8::Context> context) const;

 private:
  class FromV8ValueState;
  class ScopedUniquenessGuard;

  v8::Local<v8::Value> ToV8ValueImpl(v8::Isolate* isolate,
                                     const base::Value* value) const;
  v8::Local<v8::Value> ToV8Array(v8::Isolate* isolate,
                                 const base::ListValue* list) const;
  v8::Local<v8::Value> ToV8Object(
      v8::Isolate* isolate,
      const base::DictionaryValue* dictionary) const;
  v8::Local<v8::Value> ToArrayBuffer(v8::Isolate* isolate,
                                     const base::Value* value) const;

  std::unique_ptr<base::Value> FromV8ValueImpl(FromV8ValueState* state,
                                               v8::Local<v8::Value> value,
                                               v8::Isolate* isolate) const;
  std::unique_ptr<base::Value> FromV8Array(v8::Local<v8::Array> array,
                                           FromV8ValueState* state,
                                           v8::Isolate* isolate) const;
  std::unique_ptr<base::Value> FromNodeBuffer(v8::Local<v8::Value> value,
                                              FromV8ValueState* state,
                                              v8::Isolate* isolate) const;
  std::unique_ptr<base::Value> FromV8Object(v8::Local<v8::Object> object,
                                            FromV8ValueState* state,
                                            v8::Isolate* isolate) const;

  // 如果为true，我们将RegExp JavaScript对象转换为字符串。
  bool reg_exp_allowed_ = false;

  // 如果为true，我们将把函数JavaScript对象转换为字典。
  bool function_allowed_ = false;

  // 如果为true，则在转换V8对象时忽略未定义和空值。
  // 转化为价值。
  bool strip_null_from_objects_ = false;

  DISALLOW_COPY_AND_ASSIGN(V8ValueConverter);
};

}  // 命名空间电子。

#endif  // Shell_COMMON_V8_VALUE_转换器_H_
