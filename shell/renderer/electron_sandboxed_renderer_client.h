// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。
#ifndef SHELL_RENDERER_ELECTRON_SANDBOXED_RENDERER_CLIENT_H_
#define SHELL_RENDERER_ELECTRON_SANDBOXED_RENDERER_CLIENT_H_

#include <memory>
#include <set>
#include <string>

#include "shell/renderer/renderer_client_base.h"

namespace base {
class ProcessMetrics;
}

namespace blink {
class WebLocalFrame;
}

namespace electron {

class ElectronSandboxedRendererClient : public RendererClientBase {
 public:
  ElectronSandboxedRendererClient();
  ~ElectronSandboxedRendererClient() override;

  void InitializeBindings(v8::Local<v8::Object> binding,
                          v8::Local<v8::Context> context,
                          content::RenderFrame* render_frame);
  // 电子邮件：：RendererClientBase：
  void DidCreateScriptContext(v8::Handle<v8::Context> context,
                              content::RenderFrame* render_frame) override;
  void WillReleaseScriptContext(v8::Handle<v8::Context> context,
                                content::RenderFrame* render_frame) override;
  // 内容：：ContentRendererClient：
  void RenderFrameCreated(content::RenderFrame*) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;

 private:
  std::unique_ptr<base::ProcessMetrics> metrics_;

  // 从Web框架获取主脚本上下文将延迟初始化。
  // 其脚本上下文。在没有脚本的网页中执行此操作将触发。
  // 断言，所以我们必须保留一本关于注入的网页框架的书。
  std::set<content::RenderFrame*> injected_frames_;

  DISALLOW_COPY_AND_ASSIGN(ElectronSandboxedRendererClient);
};

}  // 命名空间电子。

#endif  // SHELL_RENDERER_ELECTRON_SANDBOXED_RENDERER_CLIENT_H_
