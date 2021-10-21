// 版权所有(C)2020 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_WIN_DIALOG_THREAD_H_
#define SHELL_BROWSER_UI_WIN_DIALOG_THREAD_H_

#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_thread.h"

namespace dialog_thread {

// 返回对话框所在的专用单线程序列。
using TaskRunner = scoped_refptr<base::SingleThreadTaskRunner>;
TaskRunner CreateDialogTaskRunner();

// 运行对话框线程中的|Execute|，并将结果传递给UI线程中的|Done|。
template <typename R>
void Run(base::OnceCallback<R()> execute, base::OnceCallback<void(R)> done) {
  // DialogThread.postTask(()=&gt;{。
  // R=执行()。
  // UiThread.postTask(()=&gt;{。
  // 完成(R)。
  // }。
  // })。
  TaskRunner task_runner = CreateDialogTaskRunner();
  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](TaskRunner task_runner, base::OnceCallback<R()> execute,
             base::OnceCallback<void(R)> done) {
            R r = std::move(execute).Run();
            base::PostTask(
                FROM_HERE, {content::BrowserThread::UI},
                base::BindOnce(
                    [](TaskRunner task_runner, base::OnceCallback<void(R)> done,
                       R r) {
                      std::move(done).Run(std::move(r));
                      // 任务运行器将在。
                      // 作用域结束。
                    },
                    std::move(task_runner), std::move(done), std::move(r)));
          },
          std::move(task_runner), std::move(execute), std::move(done)));
}

// 用于处理返回bool的|Execute|的适配器。
template <typename R>
void Run(base::OnceCallback<bool(R*)> execute,
         base::OnceCallback<void(bool, R)> done) {
  // 运行(()=&gt;{。
  // 结果=EXECUTE(&VALUE)。
  // 返回{Result，Value}。
  // }，({结果，值})=&gt;{。
  // 完成(结果、值)。
  // })。
  struct Result {
    bool result;
    R value;
  };
  Run(base::BindOnce(
          [](base::OnceCallback<bool(R*)> execute) {
            Result r;
            r.result = std::move(execute).Run(&r.value);
            return r;
          },
          std::move(execute)),
      base::BindOnce(
          [](base::OnceCallback<void(bool, R)> done, Result r) {
            std::move(done).Run(r.result, std::move(r.value));
          },
          std::move(done)));
}

}  // 命名空间对话框_线程。

#endif  // Shell_Browser_UI_Win_DIALOG_THREAD_H_
