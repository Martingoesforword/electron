// 版权所有(C)2019 GitHub，Inc.保留所有权利。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_helper/object_template_builder.h"

namespace gin_helper {

ObjectTemplateBuilder::ObjectTemplateBuilder(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ)
    : isolate_(isolate), template_(templ) {}

ObjectTemplateBuilder& ObjectTemplateBuilder::SetImpl(
    const base::StringPiece& name,
    v8::Local<v8::Data> val) {
  template_->Set(gin::StringToSymbol(isolate_, name), val);
  return *this;
}

ObjectTemplateBuilder& ObjectTemplateBuilder::SetPropertyImpl(
    const base::StringPiece& name,
    v8::Local<v8::FunctionTemplate> getter,
    v8::Local<v8::FunctionTemplate> setter) {
  template_->SetAccessorProperty(gin::StringToSymbol(isolate_, name), getter,
                                 setter);
  return *this;
}

v8::Local<v8::ObjectTemplate> ObjectTemplateBuilder::Build() {
  v8::Local<v8::ObjectTemplate> result = template_;
  template_.Clear();
  return result;
}

}  // 命名空间gin_helper
