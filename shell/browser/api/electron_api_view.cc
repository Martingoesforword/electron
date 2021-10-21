// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_view.h"

#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

View::View(views::View* view) : view_(view) {
  view_->set_owned_by_client();
}

View::View() : View(new views::View()) {}

View::~View() {
  if (delete_view_)
    delete view_;
}

#if BUILDFLAG(ENABLE_VIEWS_API)
void View::AddChildView(gin::Handle<View> child) {
  AddChildViewAt(child, child_views_.size());
}

void View::AddChildViewAt(gin::Handle<View> child, size_t index) {
  if (index > child_views_.size())
    return;
  child_views_.emplace(child_views_.begin() + index,     // 指标。
                       isolate(), child->GetWrapper());  // V8：：GLOBAL(参数...)。
  view()->AddChildViewAt(child->view(), index);
}
#endif

// 静电。
gin_helper::WrappableBase* View::New(gin::Arguments* args) {
  auto* view = new View();
  view->InitWithArgs(args);
  return view;
}

// 静电。
void View::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "View"));
#if BUILDFLAG(ENABLE_VIEWS_API)
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("addChildView", &View::AddChildView)
      .SetMethod("addChildViewAt", &View::AddChildViewAt);
#endif
}

}  // 命名空间API。

}  // 命名空间电子。

namespace {

using electron::api::View;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  View::SetConstructor(isolate, base::BindRepeating(&View::New));

  gin_helper::Dictionary constructor(
      isolate,
      View::GetConstructor(isolate)->GetFunction(context).ToLocalChecked());

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("View", constructor);
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_view, Initialize)
