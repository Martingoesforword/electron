// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_NODE_BINDINGS_WIN_H_
#define SHELL_COMMON_NODE_BINDINGS_WIN_H_

#include "base/compiler_specific.h"
#include "shell/common/node_bindings.h"

namespace electron {

class NodeBindingsWin : public NodeBindings {
 public:
  explicit NodeBindingsWin(BrowserEnvironment browser_env);
  ~NodeBindingsWin() override;

 private:
  void PollEvents() override;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsWin);
};

}  // 命名空间电子。

#endif  // Shell_COMMON_NODE_BINDINGS_WIN_H_
