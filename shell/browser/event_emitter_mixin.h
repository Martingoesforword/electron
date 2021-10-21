// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_
#define SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_

#include <utility>

#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/event_emitter.h"

namespace gin_helper {

namespace internal {
v8::Local<v8::FunctionTemplate> GetEventEmitterTemplate(v8::Isolate* isolate);
}  // 命名空间内部。

template <typename T>
class EventEmitterMixin {
 public:
  // This.emit(name，new event()，args...)；
  // 如果在处理过程中调用event.prevenentDefault()，则返回TRUE。
  template <typename... Args>
  bool Emit(base::StringPiece name, Args&&... args) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Object> wrapper;
    if (!static_cast<T*>(this)->GetWrapper(isolate).ToLocal(&wrapper))
      return false;
    v8::Local<v8::Object> event = internal::CreateCustomEvent(isolate, wrapper);
    return EmitWithEvent(isolate, wrapper, name, event,
                         std::forward<Args>(args)...);
  }

  // This.emit(名称，事件，参数...)；
  template <typename... Args>
  bool EmitCustomEvent(base::StringPiece name,
                       v8::Local<v8::Object> custom_event,
                       Args&&... args) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Object> wrapper;
    if (!static_cast<T*>(this)->GetWrapper(isolate).ToLocal(&wrapper))
      return false;
    return EmitWithEvent(isolate, wrapper, name, custom_event,
                         std::forward<Args>(args)...);
  }

 protected:
  EventEmitterMixin() = default;

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(v8::Isolate* isolate) {
    gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
    auto* wrapper_info = &(static_cast<T*>(this)->kWrapperInfo);
    v8::Local<v8::FunctionTemplate> constructor =
        data->GetFunctionTemplate(wrapper_info);
    if (constructor.IsEmpty()) {
      constructor = v8::FunctionTemplate::New(isolate);
      constructor->SetClassName(
          gin::StringToV8(isolate, static_cast<T*>(this)->GetTypeName()));
      constructor->Inherit(internal::GetEventEmitterTemplate(isolate));
      data->SetFunctionTemplate(wrapper_info, constructor);
    }
    return gin::ObjectTemplateBuilder(isolate,
                                      static_cast<T*>(this)->GetTypeName(),
                                      constructor->InstanceTemplate());
  }

 private:
  // This.emit(名称，事件，参数...)；
  template <typename... Args>
  static bool EmitWithEvent(v8::Isolate* isolate,
                            v8::Local<v8::Object> wrapper,
                            base::StringPiece name,
                            v8::Local<v8::Object> event,
                            Args&&... args) {
    auto context = isolate->GetCurrentContext();
    gin_helper::EmitEvent(isolate, wrapper, name, event,
                          std::forward<Args>(args)...);
    v8::Local<v8::Value> defaultPrevented;
    if (event->Get(context, gin::StringToV8(isolate, "defaultPrevented"))
            .ToLocal(&defaultPrevented)) {
      return defaultPrevented->BooleanValue(isolate);
    }
    return false;
  }

  DISALLOW_COPY_AND_ASSIGN(EventEmitterMixin);
};

}  // 命名空间gin_helper。

#endif  // Shell_Browser_Event_Emitter_Mixin_H_
