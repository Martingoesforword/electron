// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_MICROTASKS_SCOPE_H_
#define SHELL_COMMON_GIN_HELPER_MICROTASKS_SCOPE_H_

#include <memory>

#include "base/macros.h"
#include "v8/include/v8.h"

namespace gin_helper {

// 在浏览器进程中运行v8：：MicrotasksScope：：PerformCheckpoint。
// 在渲染过程中创建V8：：MicrotasksScope。
class MicrotasksScope {
 public:
  explicit MicrotasksScope(v8::Isolate* isolate,
                           bool ignore_browser_checkpoint = false,
                           v8::MicrotasksScope::Type scope_type =
                               v8::MicrotasksScope::kRunMicrotasks);
  ~MicrotasksScope();

 private:
  std::unique_ptr<v8::MicrotasksScope> v8_microtasks_scope_;

  DISALLOW_COPY_AND_ASSIGN(MicrotasksScope);
};

}  // 命名空间gin_helper。

#endif  // Shell_COMMON_GIN_HELPER_MicroTasks_Scope_H_
