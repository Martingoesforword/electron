// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_MICROTASKS_RUNNER_H_
#define SHELL_BROWSER_MICROTASKS_RUNNER_H_

#include "base/task/task_observer.h"

namespace v8 {
class Isolate;
}

namespace electron {

// 微任务(如Promise Resolve)在当前。
// 任务。这个类实现了一个运行的任务观察器，它告诉V8运行它们。
// 微任务运行器的实现瞬间基于EndOfTaskRunner。
// 节点遵循kExpltual MicrotasksPolicy，我们在浏览器中执行相同的操作。
// 进程。因此，我们需要让这个任务观察器刷新排队的。
// 微任务。
class MicrotasksRunner : public base::TaskObserver {
 public:
  explicit MicrotasksRunner(v8::Isolate* isolate);

  // 基地：：任务观察者。
  void WillProcessTask(const base::PendingTask& pending_task,
                       bool was_blocked_or_low_priority) override;
  void DidProcessTask(const base::PendingTask& pending_task) override;

 private:
  v8::Isolate* isolate_;
};

}  // 命名空间电子。

#endif  // Shell_Browser_MicroTasks_Runner_H_
