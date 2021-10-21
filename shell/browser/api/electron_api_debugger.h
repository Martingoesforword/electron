// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_DEBUGGER_H_
#define SHELL_BROWSER_API_ELECTRON_API_DEBUGGER_H_

#include <map>

#include "base/callback.h"
#include "base/values.h"
#include "content/public/browser/devtools_agent_host_client.h"
#include "content/public/browser/web_contents_observer.h"
#include "gin/arguments.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/promise.h"

namespace content {
class DevToolsAgentHost;
class WebContents;
}  // 命名空间内容。

namespace electron {

namespace api {

class Debugger : public gin::Wrappable<Debugger>,
                 public gin_helper::EventEmitterMixin<Debugger>,
                 public content::DevToolsAgentHostClient,
                 public content::WebContentsObserver {
 public:
  static gin::Handle<Debugger> Create(v8::Isolate* isolate,
                                      content::WebContents* web_contents);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  Debugger(v8::Isolate* isolate, content::WebContents* web_contents);
  ~Debugger() override;

  // 内容：：DevToolsAgentHostClient：
  void AgentHostClosed(content::DevToolsAgentHost* agent_host) override;
  void DispatchProtocolMessage(content::DevToolsAgentHost* agent_host,
                               base::span<const uint8_t> message) override;

  // 内容：：WebContentsViewer：
  void RenderFrameHostChanged(content::RenderFrameHost* old_rfh,
                              content::RenderFrameHost* new_rfh) override;

 private:
  using PendingRequestMap =
      std::map<int, gin_helper::Promise<base::DictionaryValue>>;

  void Attach(gin::Arguments* args);
  bool IsAttached();
  void Detach();
  v8::Local<v8::Promise> SendCommand(gin::Arguments* args);
  void ClearPendingRequests();

  content::WebContents* web_contents_;  // 弱引用。
  scoped_refptr<content::DevToolsAgentHost> agent_host_;

  PendingRequestMap pending_requests_;
  int previous_request_id_ = 0;

  DISALLOW_COPY_AND_ASSIGN(Debugger);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Electronics_API_Debugger_H_
