// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/event_emitter_mixin.h"

#include "gin/public/wrapper_info.h"
#include "shell/browser/api/electron_api_event_emitter.h"

namespace gin_helper {

namespace internal {

gin::WrapperInfo kWrapperInfo = {gin::kEmbedderNativeGin};

v8::Local<v8::FunctionTemplate> GetEventEmitterTemplate(v8::Isolate* isolate) {
  gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  v8::Local<v8::FunctionTemplate> tmpl =
      data->GetFunctionTemplate(&kWrapperInfo);

  if (tmpl.IsEmpty()) {
    tmpl = v8::FunctionTemplate::New(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Function> func = tmpl->GetFunction(context).ToLocalChecked();

    v8::Local<v8::Object> eventemitter_prototype =
        electron::GetEventEmitterPrototype(isolate);

    v8::Local<v8::Value> func_prototype;
    CHECK(func->Get(context, gin::StringToSymbol(isolate, "prototype"))
              .ToLocal(&func_prototype));

    CHECK(func_prototype.As<v8::Object>()
              ->SetPrototype(context, eventemitter_prototype)
              .ToChecked());

    data->SetFunctionTemplate(&kWrapperInfo, tmpl);
  }

  return tmpl;
}

}  // 命名空间内部。

}  // 命名空间gin_helper
