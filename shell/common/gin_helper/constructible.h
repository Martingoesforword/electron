// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_
#define SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_

#include "gin/per_isolate_data.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/function_template_extensions.h"

namespace gin_helper {
template <typename T>
class EventEmitterMixin;

// 应使用“new”构造的可包装对象的帮助器类。
// 用JavaScript编写。
// 
// 要使用，请继承自gin：：Wrappable和gin_helper：：Construction，以及。
// 定义静态方法New和FillObjectTemplate：
// 
// 类示例：public gin：：Wrappable&lt;示例&gt;，
// Public gin_helper：：可构造&lt;示例&gt;{。
// 公众：
// Static gin：：Handle&lt;Tray&gt;New(...通常的gin方法参数...)；
// 静态v8：：local&lt;v8：：ObjectTemplate&gt;FillObjectTemplate(。
// V8：：Isolate*，
// V8：：local&lt;v8：：ObjectTemplate&gt;)；
// }。
// 
// 不要定义通常的gin：：Wrappable：：GetObjectTemplateBuilder。会的。
// 不能为可构造类调用。
// 
// 要公开构造函数，请调用GetConstructor：
// 
// Gin：：字典字典(隔离、导出)；
// Dic.Set(“Example”，Example：：GetConstructor(Context))；
template <typename T>
class Constructible {
 public:
  static v8::Local<v8::Function> GetConstructor(
      v8::Local<v8::Context> context) {
    v8::Isolate* isolate = context->GetIsolate();
    gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
    auto* wrapper_info = &T::kWrapperInfo;
    v8::Local<v8::FunctionTemplate> constructor =
        data->GetFunctionTemplate(wrapper_info);
    if (constructor.IsEmpty()) {
      constructor = gin::CreateConstructorFunctionTemplate(
          isolate, base::BindRepeating(&T::New));
      if (std::is_base_of<EventEmitterMixin<T>, T>::value) {
        constructor->Inherit(
            gin_helper::internal::GetEventEmitterTemplate(isolate));
      }
      constructor->InstanceTemplate()->SetInternalFieldCount(
          gin::kNumberOfInternalFields);
      v8::Local<v8::ObjectTemplate> obj_templ =
          T::FillObjectTemplate(isolate, constructor->InstanceTemplate());
      data->SetObjectTemplate(wrapper_info, obj_templ);
      data->SetFunctionTemplate(wrapper_info, constructor);
    }
    return constructor->GetFunction(context).ToLocalChecked();
  }
};

}  // 命名空间gin_helper。

#endif  // Shell_COMMON_GIN_HELPER_CONSTRUCTABLE_H_
