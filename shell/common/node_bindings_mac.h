// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_NODE_BINDINGS_MAC_H_
#define SHELL_COMMON_NODE_BINDINGS_MAC_H_

#include "base/compiler_specific.h"
#include "shell/common/node_bindings.h"

namespace electron {

class NodeBindingsMac : public NodeBindings {
 public:
  explicit NodeBindingsMac(BrowserEnvironment browser_env);
  ~NodeBindingsMac() override;

  void RunMessageLoop() override;

 private:
  // 当UV的监视器队列更改时调用。
  static void OnWatcherQueueChanged(uv_loop_t* loop);

  void PollEvents() override;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsMac);
};

}  // 命名空间电子。

#endif  // Shell_COMMON_NODE_BINDINGS_MAC_H_
