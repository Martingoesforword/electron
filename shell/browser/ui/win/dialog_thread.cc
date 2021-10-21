// 版权所有(C)2020 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/win/dialog_thread.h"

#include "base/task/thread_pool.h"

namespace dialog_thread {

// 创建要在其上运行外壳对话框的SingleThreadTaskRunner。每个对话框。
// 需要它自己的专用单线程序列，否则在某些情况下。
// 单例拥有此对象的单个实例的情况下，我们可以。
// 有这样一种情况，即一个窗口中的模式对话框阻止了外观。
// 模式对话框在另一个模式对话框中的。
TaskRunner CreateDialogTaskRunner() {
  return base::ThreadPool::CreateCOMSTATaskRunner(
      {base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN, base::MayBlock()},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED);
}

}  // 命名空间对话框_线程
