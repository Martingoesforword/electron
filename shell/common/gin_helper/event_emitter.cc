// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_helper/event_emitter.h"

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "shell/browser/api/event.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"

namespace gin_helper {

namespace internal {

namespace {

v8::Persistent<v8::ObjectTemplate> event_template;

void PreventDefault(gin_helper::Arguments* args) {
  Dictionary self;
  if (args->GetHolder(&self))
    self.Set("defaultPrevented", true);
}

}  // 命名空间。

v8::Local<v8::Object> CreateCustomEvent(v8::Isolate* isolate,
                                        v8::Local<v8::Object> sender,
                                        v8::Local<v8::Object> custom_event) {
  if (event_template.IsEmpty()) {
    event_template.Reset(
        isolate,
        ObjectTemplateBuilder(isolate, v8::ObjectTemplate::New(isolate))
            .SetMethod("preventDefault", &PreventDefault)
            .Build());
  }

  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  CHECK(!context.IsEmpty());
  v8::Local<v8::Object> event =
      v8::Local<v8::ObjectTemplate>::New(isolate, event_template)
          ->NewInstance(context)
          .ToLocalChecked();
  if (!sender.IsEmpty())
    Dictionary(isolate, event).Set("sender", sender);
  if (!custom_event.IsEmpty())
    event->SetPrototype(context, custom_event).IsJust();
  return event;
}

v8::Local<v8::Object> CreateNativeEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> sender,
    content::RenderFrameHost* frame,
    electron::mojom::ElectronBrowser::MessageSyncCallback callback) {
  v8::Local<v8::Object> event;
  if (frame && callback) {
    gin::Handle<Event> native_event = Event::Create(isolate);
    native_event->SetCallback(std::move(callback));
    event = native_event.ToV8().As<v8::Object>();
  } else {
    // 如果我们不需要发送回复，则不需要创建本地事件。
    event = CreateCustomEvent(isolate);
  }

  Dictionary dict(isolate, event);
  dict.Set("sender", sender);
  // 即使回调为空，也应始终设置frame ID。
  if (frame) {
    dict.Set("frameId", frame->GetRoutingID());
    dict.Set("processId", frame->GetProcess()->GetID());
  }
  return event;
}

}  // 命名空间内部。

}  // 命名空间gin_helper
