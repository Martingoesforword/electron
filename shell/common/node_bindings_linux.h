// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_NODE_BINDINGS_LINUX_H_
#define SHELL_COMMON_NODE_BINDINGS_LINUX_H_

#include "base/compiler_specific.h"
#include "shell/common/node_bindings.h"

namespace electron {

class NodeBindingsLinux : public NodeBindings {
 public:
  explicit NodeBindingsLinux(BrowserEnvironment browser_env);
  ~NodeBindingsLinux() override;

  void RunMessageLoop() override;

 private:
  // 当UV的监视器队列更改时调用。
  static void OnWatcherQueueChanged(uv_loop_t* loop);

  void PollEvents() override;

  // 轮询UV的后端FD的EPOLL。
  int epoll_;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsLinux);
};

}  // 命名空间电子。

#endif  // Shell_COMMON_NODE_BINDINGS_Linux_H_
