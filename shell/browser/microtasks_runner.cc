
// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/microtasks_runner.h"

#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

namespace electron {

MicrotasksRunner::MicrotasksRunner(v8::Isolate* isolate) : isolate_(isolate) {}

void MicrotasksRunner::WillProcessTask(const base::PendingTask& pending_task,
                                       bool was_blocked_or_low_priority) {}

void MicrotasksRunner::DidProcessTask(const base::PendingTask& pending_task) {
  v8::Isolate::Scope scope(isolate_);
  // 在浏览器进程中，我们遵循kExplict的Node.js微任务策略。
  // 并让作为Chrome UI任务观察器的MicrotaskRunner线程。
  // 计划程序运行微任务检查点。这很好用，因为Node.js。
  // 也通过其任务队列运行微任务，但在。
  // Https://github.com/electron/electron/issues/20013 Node.js现在执行其。
  // 自己的微任务检查点，它可能会发生在某些情况下。
  // 在Node.js和Chrome之间争用检查点，结束。
  // Up Node.js终止其回调。为了解决这个问题，现在我们总是让Node.js。
  // 在浏览器进程中处理检查点。
  {
    v8::HandleScope scope(isolate_);
    node::CallbackScope microtasks_scope(isolate_, v8::Object::New(isolate_),
                                         {0, 0});
  }
}

}  // 命名空间电子
