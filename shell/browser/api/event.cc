// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/event.h"

#include <utility>

#include "gin/data_object_builder.h"
#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/std_converter.h"

namespace gin_helper {

gin::WrapperInfo Event::kWrapperInfo = {gin::kEmbedderNativeGin};

Event::Event() = default;

Event::~Event() {
  if (callback_) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    // 如果没有当前的上下文，那就意味着我们要关门了，所以我们不会。
    // 需要发送事件。
    if (!isolate->GetCurrentContext().IsEmpty()) {
      v8::HandleScope scope(isolate);
      auto message = gin::DataObjectBuilder(isolate)
                         .Set("error", "reply was never sent")
                         .Build();
      SendReply(isolate, message);
    }
  }
}

void Event::SetCallback(InvokeCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
}

void Event::PreventDefault(v8::Isolate* isolate) {
  v8::Local<v8::Object> self = GetWrapper(isolate).ToLocalChecked();
  self->Set(isolate->GetCurrentContext(),
            gin::StringToV8(isolate, "defaultPrevented"), v8::True(isolate))
      .Check();
}

bool Event::SendReply(v8::Isolate* isolate, v8::Local<v8::Value> result) {
  if (!callback_)
    return false;

  blink::CloneableMessage message;
  if (!gin::ConvertFromV8(isolate, result, &message)) {
    return false;
  }

  std::move(callback_).Run(std::move(message));
  return true;
}

gin::ObjectTemplateBuilder Event::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<Event>::GetObjectTemplateBuilder(isolate)
      .SetMethod("preventDefault", &Event::PreventDefault)
      .SetMethod("sendReply", &Event::SendReply);
}

const char* Event::GetTypeName() {
  return "Event";
}

// 静电。
gin::Handle<Event> Event::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new Event());
}

}  // 命名空间gin_helper
