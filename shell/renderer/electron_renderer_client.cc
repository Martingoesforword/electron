// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/electron_renderer_client.h"

#include <string>

#include "base/command_line.h"
#include "content/public/renderer/render_frame.h"
#include "electron/buildflags/buildflags.h"
#include "net/http/http_request_headers.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "shell/renderer/web_worker_observer.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace {

bool IsDevToolsExtension(content::RenderFrame* render_frame) {
  return static_cast<GURL>(render_frame->GetWebFrame()->GetDocument().Url())
      .SchemeIs("chrome-extension");
}

}  // 命名空间。

ElectronRendererClient::ElectronRendererClient()
    : node_bindings_(
          NodeBindings::Create(NodeBindings::BrowserEnvironment::kRenderer)),
      electron_bindings_(
          std::make_unique<ElectronBindings>(node_bindings_->uv_loop())) {}

ElectronRendererClient::~ElectronRendererClient() = default;

void ElectronRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new ElectronRenderFrameObserver(render_frame, this);
  RendererClientBase::RenderFrameCreated(render_frame);
}

void ElectronRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentStart(render_frame);
  // 通知文件开始发声。
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  node::Environment* env = GetEnvironment(render_frame);
  if (env)
    gin_helper::EmitEvent(env->isolate(), env->process_object(),
                          "document-start");
}

void ElectronRendererClient::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
  RendererClientBase::RunScriptsAtDocumentEnd(render_frame);
  // 通知文件结束时的声音。
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  node::Environment* env = GetEnvironment(render_frame);
  if (env)
    gin_helper::EmitEvent(env->isolate(), env->process_object(),
                          "document-end");
}

void ElectronRendererClient::DidCreateScriptContext(
    v8::Handle<v8::Context> renderer_context,
    content::RenderFrame* render_frame) {
  // TODO(Zcbenz)：如果未进行节点集成，则不创建节点环境。
  // 已启用。

  // 仅当我们是主机或DevTools扩展时才加载Node.js。
  // 除非已为子帧显式启用了Node.js支持。
  auto prefs = render_frame->GetBlinkPreferences();
  bool is_main_frame = render_frame->IsMainFrame();
  bool is_devtools = IsDevToolsExtension(render_frame);
  bool allow_node_in_subframes = prefs.node_integration_in_sub_frames;

  bool should_load_node =
      (is_main_frame || is_devtools || allow_node_in_subframes) &&
      !IsWebViewFrame(renderer_context, render_frame);
  if (!should_load_node)
    return;

  injected_frames_.insert(render_frame);

  if (!node_integration_initialized_) {
    node_integration_initialized_ = true;
    node_bindings_->Initialize();
    node_bindings_->PrepareMessageLoop();
  } else {
    node_bindings_->PrepareMessageLoop();
  }

  // 设置节点跟踪控制器。
  if (!node::tracing::TraceEventHelper::GetAgent())
    node::tracing::TraceEventHelper::SetAgent(node::CreateAgent());

  // 为每个窗口设置节点环境。
  bool initialized = node::InitializeContext(renderer_context);
  CHECK(initialized);

  node::Environment* env =
      node_bindings_->CreateEnvironment(renderer_context, nullptr);

  // 如果我们已禁用站点实例覆盖，则应阻止加载。
  // 任何非上下文感知的本机模块。
  env->options()->force_context_aware = true;

  // 我们不想让渲染器进程因未处理的拒绝而崩溃。
  env->options()->unhandled_rejections = "warn";

  environments_.insert(env);

  // 添加电子扩展API。
  electron_bindings_->BindTo(env->isolate(), env->process_object());
  gin_helper::Dictionary process_dict(env->isolate(), env->process_object());
  BindProcess(env->isolate(), &process_dict, render_frame);

  // 把所有东西都装上。
  node_bindings_->LoadEnvironment(env);

  if (node_bindings_->uv_env() == nullptr) {
    // 使UV循环被窗口上下文包裹。
    node_bindings_->set_uv_env(env);

    // 让节点循环运行一次，以确保一切准备就绪。
    node_bindings_->RunMessageLoop();
  }
}

void ElectronRendererClient::WillReleaseScriptContext(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  if (injected_frames_.erase(render_frame) == 0)
    return;

  node::Environment* env = node::Environment::GetCurrent(context);
  if (environments_.erase(env) == 0)
    return;

  gin_helper::EmitEvent(env->isolate(), env->process_object(), "exit");

  // 主机可能会被更换。
  if (env == node_bindings_->uv_env())
    node_bindings_->set_uv_env(nullptr);

  // 销毁节点环境。我们仅在节点支持已完成的情况下才执行此操作。
  // 为子帧启用以避免行为改变/引入崩溃。
  // 适用于现有用户。
  // 如果我们将电子站点实例覆盖禁用为。
  // 避免内存泄漏。
  auto prefs = render_frame->GetBlinkPreferences();
  node::FreeEnvironment(env);
  if (env == node_bindings_->uv_env())
    node::FreeIsolateData(node_bindings_->isolate_data());

  // 电子绑定正在跟踪节点环境。
  electron_bindings_->EnvironmentDestroyed(env);
}

void ElectronRendererClient::WorkerScriptReadyForEvaluationOnWorkerThread(
    v8::Local<v8::Context> context) {
  // TODO(Loc)：请注意，这对于进程中的子窗口是不正确的。
  // 具有不同nodeIntegrationInWorker值的webPreferences。
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInWorker)) {
    WebWorkerObserver::GetCurrent()->WorkerScriptReadyForEvaluation(context);
  }
}

void ElectronRendererClient::WillDestroyWorkerContextOnWorkerThread(
    v8::Local<v8::Context> context) {
  // TODO(Loc)：请注意，这对于进程中的子窗口是不正确的。
  // 具有不同nodeIntegrationInWorker值的webPreferences。
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNodeIntegrationInWorker)) {
    WebWorkerObserver::GetCurrent()->ContextWillDestroy(context);
  }
}

node::Environment* ElectronRendererClient::GetEnvironment(
    content::RenderFrame* render_frame) const {
  if (injected_frames_.find(render_frame) == injected_frames_.end())
    return nullptr;
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  auto context =
      GetContext(render_frame->GetWebFrame(), v8::Isolate::GetCurrent());
  node::Environment* env = node::Environment::GetCurrent(context);
  if (environments_.find(env) == environments_.end())
    return nullptr;
  return env;
}

}  // 命名空间电子
