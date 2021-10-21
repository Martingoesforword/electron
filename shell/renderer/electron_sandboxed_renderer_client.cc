// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/electron_sandboxed_renderer_client.h"

#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/process/process_handle.h"
#include "base/process/process_metrics.h"
#include "content/public/renderer/render_frame.h"
#include "electron/buildflags/buildflags.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/options_switches.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/electron_node/src/node_binding.h"

namespace electron {

namespace {

const char kLifecycleKey[] = "lifecycle";
const char kModuleCacheKey[] = "native-module-cache";

bool IsDevTools(content::RenderFrame* render_frame) {
  return render_frame->GetWebFrame()->GetDocument().Url().ProtocolIs(
      "devtools");
}

bool IsDevToolsExtension(content::RenderFrame* render_frame) {
  return render_frame->GetWebFrame()->GetDocument().Url().ProtocolIs(
      "chrome-extension");
}

v8::Local<v8::Object> GetModuleCache(v8::Isolate* isolate) {
  auto context = isolate->GetCurrentContext();
  gin_helper::Dictionary global(isolate, context->Global());
  v8::Local<v8::Value> cache;

  if (!global.GetHidden(kModuleCacheKey, &cache)) {
    cache = v8::Object::New(isolate);
    global.SetHidden(kModuleCacheKey, cache);
  }

  return cache->ToObject(context).ToLocalChecked();
}

// 改编自node.cc。
v8::Local<v8::Value> GetBinding(v8::Isolate* isolate,
                                v8::Local<v8::String> key,
                                gin_helper::Arguments* margs) {
  v8::Local<v8::Object> exports;
  std::string module_key = gin::V8ToString(isolate, key);
  gin_helper::Dictionary cache(isolate, GetModuleCache(isolate));

  if (cache.Get(module_key.c_str(), &exports)) {
    return exports;
  }

  auto* mod = node::binding::get_linked_module(module_key.c_str());

  if (!mod) {
    char errmsg[1024];
    snprintf(errmsg, sizeof(errmsg), "No such module: %s", module_key.c_str());
    margs->ThrowError(errmsg);
    return exports;
  }

  exports = v8::Object::New(isolate);
  DCHECK_EQ(mod->nm_register_func, nullptr);
  DCHECK_NE(mod->nm_context_register_func, nullptr);
  mod->nm_context_register_func(exports, v8::Null(isolate),
                                isolate->GetCurrentContext(), mod->nm_priv);
  cache.Set(module_key.c_str(), exports);
  return exports;
}

v8::Local<v8::Value> CreatePreloadScript(v8::Isolate* isolate,
                                         v8::Local<v8::String> source) {
  auto context = isolate->GetCurrentContext();
  auto maybe_script = v8::Script::Compile(context, source);
  v8::Local<v8::Script> script;
  if (!maybe_script.ToLocal(&script))
    return v8::Local<v8::Value>();
  return script->Run(context).ToLocalChecked();
}

double Uptime() {
  return (base::Time::Now() - base::Process::Current().CreationTime())
      .InSecondsF();
}

void InvokeHiddenCallback(v8::Handle<v8::Context> context,
                          const std::string& hidden_key,
                          const std::string& callback_name) {
  auto* isolate = context->GetIsolate();
  auto binding_key =
      gin::ConvertToV8(isolate, hidden_key)->ToString(context).ToLocalChecked();
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  auto global_object = context->Global();
  v8::Local<v8::Value> value;
  if (!global_object->GetPrivate(context, private_binding_key).ToLocal(&value))
    return;
  if (value.IsEmpty() || !value->IsObject())
    return;
  auto binding = value->ToObject(context).ToLocalChecked();
  auto callback_key = gin::ConvertToV8(isolate, callback_name)
                          ->ToString(context)
                          .ToLocalChecked();
  auto callback_value = binding->Get(context, callback_key).ToLocalChecked();
  DCHECK(callback_value->IsFunction());  // 由沙盒渲染器/init.js设置。
  auto callback = callback_value.As<v8::Function>();
  ignore_result(callback->Call(context, binding, 0, nullptr));
}

}  // 命名空间。

ElectronSandboxedRendererClient::ElectronSandboxedRendererClient() {
  // 明确注册电子的内置模块。
  NodeBindings::RegisterBuiltinModules();
  metrics_ = base::ProcessMetrics::CreateCurrentProcessMetrics();
}

ElectronSandboxedRendererClient::~ElectronSandboxedRendererClient() = default;

void ElectronSandboxedRendererClient::InitializeBindings(
    v8::Local<v8::Object> binding,
    v8::Local<v8::Context> context,
    content::RenderFrame* render_frame) {
  auto* isolate = context->GetIsolate();
  gin_helper::Dictionary b(isolate, binding);
  b.SetMethod("get", GetBinding);
  b.SetMethod("createPreloadScript", CreatePreloadScript);

  gin_helper::Dictionary process = gin::Dictionary::CreateEmpty(isolate);
  b.Set("process", process);

  ElectronBindings::BindProcess(isolate, &process, metrics_.get());
  BindProcess(isolate, &process, render_frame);

  process.SetMethod("uptime", Uptime);
  process.Set("argv", base::CommandLine::ForCurrentProcess()->argv());
  process.SetReadOnly("pid", base::GetCurrentProcId());
  process.SetReadOnly("sandboxed", true);
  process.SetReadOnly("type", "renderer");
}

void ElectronSandboxedRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new ElectronRenderFrameObserver(render_frame, this);
  RendererClientBase::RenderFrameCreated(render_frame);
}

void ElectronSandboxedRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentStart(render_frame);
  if (injected_frames_.find(render_frame) == injected_frames_.end())
    return;

  auto* isolate = blink::MainThreadIsolate();
  gin_helper::MicrotasksScope microtasks_scope(
      isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context =
      GetContext(render_frame->GetWebFrame(), isolate);
  v8::Context::Scope context_scope(context);

  InvokeHiddenCallback(context, kLifecycleKey, "onDocumentStart");
}

void ElectronSandboxedRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentEnd(render_frame);
  if (injected_frames_.find(render_frame) == injected_frames_.end())
    return;

  auto* isolate = blink::MainThreadIsolate();
  gin_helper::MicrotasksScope microtasks_scope(
      isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Context> context =
      GetContext(render_frame->GetWebFrame(), isolate);
  v8::Context::Scope context_scope(context);

  InvokeHiddenCallback(context, kLifecycleKey, "onDocumentEnd");
}

void ElectronSandboxedRendererClient::DidCreateScriptContext(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  // 仅允许对主机进行预加载，或者。
  // 对于DevTools，我们仍然希望运行preload_Bundle脚本。
  // 或者在子帧中显式启用nodeSupport时。
  bool is_main_frame = render_frame->IsMainFrame();
  bool is_devtools =
      IsDevTools(render_frame) || IsDevToolsExtension(render_frame);

  bool allow_node_in_sub_frames =
      render_frame->GetBlinkPreferences().node_integration_in_sub_frames;

  bool should_load_preload =
      (is_main_frame || is_devtools || allow_node_in_sub_frames) &&
      !IsWebViewFrame(context, render_frame);
  if (!should_load_preload)
    return;

  injected_frames_.insert(render_frame);

  // 将捆绑包包装到一个函数中，该函数将绑定对象接收为。
  // 争论。
  auto* isolate = context->GetIsolate();
  auto binding = v8::Object::New(isolate);
  InitializeBindings(binding, context, render_frame);

  std::vector<v8::Local<v8::String>> sandbox_preload_bundle_params = {
      node::FIXED_ONE_BYTE_STRING(isolate, "binding")};

  std::vector<v8::Local<v8::Value>> sandbox_preload_bundle_args = {binding};

  util::CompileAndCall(
      isolate->GetCurrentContext(), "electron/js2c/sandbox_bundle",
      &sandbox_preload_bundle_params, &sandbox_preload_bundle_args, nullptr);

  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  InvokeHiddenCallback(context, kLifecycleKey, "onLoaded");
}

void ElectronSandboxedRendererClient::WillReleaseScriptContext(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  if (injected_frames_.erase(render_frame) == 0)
    return;

  auto* isolate = context->GetIsolate();
  gin_helper::MicrotasksScope microtasks_scope(
      isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);
  InvokeHiddenCallback(context, kLifecycleKey, "onExit");
}

}  // 命名空间电子
