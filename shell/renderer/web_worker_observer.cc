// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/web_worker_observer.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace {

static base::LazyInstance<
    base::ThreadLocalPointer<WebWorkerObserver>>::DestructorAtExit lazy_tls =
    LAZY_INSTANCE_INITIALIZER;

}  // 命名空间。

// 静电。
WebWorkerObserver* WebWorkerObserver::GetCurrent() {
  WebWorkerObserver* self = lazy_tls.Pointer()->Get();
  return self ? self : new WebWorkerObserver;
}

WebWorkerObserver::WebWorkerObserver()
    : node_bindings_(
          NodeBindings::Create(NodeBindings::BrowserEnvironment::kWorker)),
      electron_bindings_(
          std::make_unique<ElectronBindings>(node_bindings_->uv_loop())) {
  lazy_tls.Pointer()->Set(this);
}

WebWorkerObserver::~WebWorkerObserver() {
  lazy_tls.Pointer()->Set(nullptr);
  node::FreeEnvironment(node_bindings_->uv_env());
  node::FreeIsolateData(node_bindings_->isolate_data());
}

void WebWorkerObserver::WorkerScriptReadyForEvaluation(
    v8::Local<v8::Context> worker_context) {
  v8::Context::Scope context_scope(worker_context);
  auto* isolate = worker_context->GetIsolate();
  v8::MicrotasksScope microtasks_scope(
      isolate, v8::MicrotasksScope::kDoNotRunMicrotasks);

  // 启动嵌入线程。
  node_bindings_->PrepareMessageLoop();

  // 设置节点跟踪控制器。
  if (!node::tracing::TraceEventHelper::GetAgent())
    node::tracing::TraceEventHelper::SetAgent(node::CreateAgent());

  // 为每个窗口设置节点环境。
  bool initialized = node::InitializeContext(worker_context);
  CHECK(initialized);
  node::Environment* env =
      node_bindings_->CreateEnvironment(worker_context, nullptr);

  // 添加电子扩展API。
  electron_bindings_->BindTo(env->isolate(), env->process_object());

  // 把所有东西都装上。
  node_bindings_->LoadEnvironment(env);

  // 使UV循环被窗口上下文包裹。
  node_bindings_->set_uv_env(env);

  // 让节点循环运行一次，以确保一切准备就绪。
  node_bindings_->RunMessageLoop();
}

void WebWorkerObserver::ContextWillDestroy(v8::Local<v8::Context> context) {
  node::Environment* env = node::Environment::GetCurrent(context);
  if (env)
    gin_helper::EmitEvent(env->isolate(), env->process_object(), "exit");

  delete this;
}

}  // 命名空间电子
