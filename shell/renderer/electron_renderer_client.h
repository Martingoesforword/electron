// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_ELECTRON_RENDERER_CLIENT_H_
#define SHELL_RENDERER_ELECTRON_RENDERER_CLIENT_H_

#include <memory>
#include <set>
#include <string>

#include "shell/renderer/renderer_client_base.h"

namespace node {
class Environment;
}

namespace electron {

class ElectronBindings;
class NodeBindings;

class ElectronRendererClient : public RendererClientBase {
 public:
  ElectronRendererClient();
  ~ElectronRendererClient() override;

  // 电子邮件：：RendererClientBase：
  void DidCreateScriptContext(v8::Handle<v8::Context> context,
                              content::RenderFrame* render_frame) override;
  void WillReleaseScriptContext(v8::Handle<v8::Context> context,
                                content::RenderFrame* render_frame) override;

 private:
  // 内容：：ContentRendererClient：
  void RenderFrameCreated(content::RenderFrame*) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
  void WorkerScriptReadyForEvaluationOnWorkerThread(
      v8::Local<v8::Context> context) override;
  void WillDestroyWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context) override;

  node::Environment* GetEnvironment(content::RenderFrame* frame) const;

  // 节点集成是否已初始化。
  bool node_integration_initialized_ = false;

  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<ElectronBindings> electron_bindings_;

  // NODE：：Environment：：GetCurrent API在执行以下操作时不返回nullptr。
  // 是为没有node：：Environment的上下文调用的，所以我们必须保持。
  // 一本关于创造的环境的书。
  std::set<node::Environment*> environments_;

  // 从Web框架获取主脚本上下文将延迟初始化。
  // 其脚本上下文。在没有脚本的网页中执行此操作将触发。
  // 断言，所以我们必须保留一本关于注入的网页框架的书。
  std::set<content::RenderFrame*> injected_frames_;

  DISALLOW_COPY_AND_ASSIGN(ElectronRendererClient);
};

}  // 命名空间电子。

#endif  // 外壳渲染器电子渲染器客户端H_
