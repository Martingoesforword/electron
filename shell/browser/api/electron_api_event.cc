// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/event.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter.h"
#include "shell/common/node_includes.h"

namespace {

v8::Local<v8::Object> CreateWithSender(v8::Isolate* isolate,
                                       v8::Local<v8::Object> sender) {
  return gin_helper::internal::CreateCustomEvent(isolate, sender);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createWithSender", &CreateWithSender);
  dict.SetMethod("createEmpty", &gin_helper::Event::Create);
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_event, Initialize)
