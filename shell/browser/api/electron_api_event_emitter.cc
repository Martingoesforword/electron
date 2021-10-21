// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_event_emitter.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/no_destructor.h"
#include "gin/dictionary.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

namespace {

v8::Global<v8::Object>* GetEventEmitterPrototypeReference() {
  static base::NoDestructor<v8::Global<v8::Object>> event_emitter_prototype;
  return event_emitter_prototype.get();
}

void SetEventEmitterPrototype(v8::Isolate* isolate,
                              v8::Local<v8::Object> proto) {
  GetEventEmitterPrototypeReference()->Reset(isolate, proto);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();

  gin::Dictionary dict(isolate, exports);
  dict.Set("setEventEmitterPrototype",
           base::BindRepeating(&SetEventEmitterPrototype));
}

}  // 命名空间。

namespace electron {

v8::Local<v8::Object> GetEventEmitterPrototype(v8::Isolate* isolate) {
  CHECK(!GetEventEmitterPrototypeReference()->IsEmpty());
  return GetEventEmitterPrototypeReference()->Get(isolate);
}

}  // 命名空间电子

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_event_emitter, Initialize)
