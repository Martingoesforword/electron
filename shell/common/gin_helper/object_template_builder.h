// 版权所有(C)2019 GitHub，Inc.保留所有权利。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_OBJECT_TEMPLATE_BUILDER_H_
#define SHELL_COMMON_GIN_HELPER_OBJECT_TEMPLATE_BUILDER_H_

#include "shell/common/gin_helper/function_template.h"

namespace gin_helper {

// 此类的工作方式与gin：：ObjectTemplateBuilder类似，但在现有。
// 原型模板，而不是创建新模板。
// 
// 它还对函数模板使用gin_helper：：CreateFunctionTemplate。
// 支持gin_helper类型。
// 
// TODO(Zcbenz)：我们应该修补gin：：ObjectTemplateBuilder以提供相同的功能。
// 删除gin_helper/function_template.h之后的功能。
class ObjectTemplateBuilder {
 public:
  ObjectTemplateBuilder(v8::Isolate* isolate,
                        v8::Local<v8::ObjectTemplate> templ);
  ~ObjectTemplateBuilder() = default;

  // 返回非常数引用违反了Google C++风格，但我们采用了一些。
  // 此处的诗意许可，以便所有对set()的调用都可以通过‘.’
  // 接线员，排好队。
  template <typename T>
  ObjectTemplateBuilder& SetValue(const base::StringPiece& name, T val) {
    return SetImpl(name, ConvertToV8(isolate_, val));
  }

  // 在以下方法中，T和U可以是函数指针、成员函数。
  // 指针、base：：RepeatingCallback或V8：：FunctionTemplate。大多数客户端。
  // 将希望使用前两个选项中的一个。另请参阅。
  // Gin：：CreateFunctionTemplate()，用于创建原始函数模板。
  template <typename T>
  ObjectTemplateBuilder& SetMethod(const base::StringPiece& name,
                                   const T& callback) {
    return SetImpl(name, CallbackTraits<T>::CreateTemplate(isolate_, callback));
  }
  template <typename T>
  ObjectTemplateBuilder& SetProperty(const base::StringPiece& name,
                                     const T& getter) {
    return SetPropertyImpl(name,
                           CallbackTraits<T>::CreateTemplate(isolate_, getter),
                           v8::Local<v8::FunctionTemplate>());
  }
  template <typename T, typename U>
  ObjectTemplateBuilder& SetProperty(const base::StringPiece& name,
                                     const T& getter,
                                     const U& setter) {
    return SetPropertyImpl(name,
                           CallbackTraits<T>::CreateTemplate(isolate_, getter),
                           CallbackTraits<U>::CreateTemplate(isolate_, setter));
  }

  v8::Local<v8::ObjectTemplate> Build();

 private:
  ObjectTemplateBuilder& SetImpl(const base::StringPiece& name,
                                 v8::Local<v8::Data> val);
  ObjectTemplateBuilder& SetPropertyImpl(
      const base::StringPiece& name,
      v8::Local<v8::FunctionTemplate> getter,
      v8::Local<v8::FunctionTemplate> setter);

  v8::Isolate* isolate_;

  // ObjectTemplateBuilder只能在堆栈上使用。
  v8::Local<v8::ObjectTemplate> template_;
};

}  // 命名空间gin_helper。

#endif  // SHELL_COMMON_GIN_HELPER_OBJECT_TEMPLATE_BUILDER_H_
