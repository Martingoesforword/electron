// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_web_contents_view.h"

#include "base/no_destructor.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/browser.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

#if defined(OS_MAC)
#include "shell/browser/ui/cocoa/delayed_native_view_host.h"
#endif

namespace electron {

namespace api {

WebContentsView::WebContentsView(v8::Isolate* isolate,
                                 gin::Handle<WebContents> web_contents)
#if defined(OS_MAC)
    : View(new DelayedNativeViewHost(web_contents->inspectable_web_contents()
                                         ->GetView()
                                         ->GetNativeView())),
#else
    : View(web_contents->inspectable_web_contents()->GetView()->GetView()),
#endif
      web_contents_(isolate, web_contents.ToV8()),
      api_web_contents_(web_contents.get()) {
#if !defined(OS_MAC)
  // 在MacOS上，View是一个新创建的|DelayedNativeViewHost|，它是我们的。
  // 有责任删除它。在其他平台上创建视图，并且。
  // 由InspectableWebContents管理。
  set_delete_view(false);
#endif
  Observe(web_contents->web_contents());
}

WebContentsView::~WebContentsView() {
  if (api_web_contents_)  // 在未关闭WebContents的情况下调用了Destroy()。
    api_web_contents_->Destroy();
}

gin::Handle<WebContents> WebContentsView::GetWebContents(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, api_web_contents_);
}

void WebContentsView::WebContentsDestroyed() {
  api_web_contents_ = nullptr;
  web_contents_.Reset();
}

// 静电。
gin::Handle<WebContentsView> WebContentsView::Create(
    v8::Isolate* isolate,
    const gin_helper::Dictionary& web_preferences) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> arg = gin::ConvertToV8(isolate, web_preferences);
  v8::Local<v8::Object> obj;
  if (GetConstructor(isolate)->NewInstance(context, 1, &arg).ToLocal(&obj)) {
    gin::Handle<WebContentsView> web_contents_view;
    if (gin::ConvertFromV8(isolate, obj, &web_contents_view))
      return web_contents_view;
  }
  return gin::Handle<WebContentsView>();
}

// 静电。
v8::Local<v8::Function> WebContentsView::GetConstructor(v8::Isolate* isolate) {
  static base::NoDestructor<v8::Global<v8::Function>> constructor;
  if (constructor.get()->IsEmpty()) {
    constructor->Reset(
        isolate, gin_helper::CreateConstructor<WebContentsView>(
                     isolate, base::BindRepeating(&WebContentsView::New)));
  }
  return v8::Local<v8::Function>::New(isolate, *constructor.get());
}

// 静电。
gin_helper::WrappableBase* WebContentsView::New(
    gin_helper::Arguments* args,
    const gin_helper::Dictionary& web_preferences) {
  auto web_contents =
      WebContents::CreateFromWebPreferences(args->isolate(), web_preferences);

  // 构造函数调用。
  auto* view = new WebContentsView(args->isolate(), web_contents);
  view->InitWithArgs(args);
  return view;
}

// 静电。
void WebContentsView::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "WebContentsView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetProperty("webContents", &WebContentsView::GetWebContents);
}

}  // 命名空间API。

}  // 命名空间电子。

namespace {

using electron::api::WebContentsView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("WebContentsView", WebContentsView::GetConstructor(isolate));
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_web_contents_view, Initialize)
