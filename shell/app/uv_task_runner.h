// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_APP_UV_TASK_RUNNER_H_
#define SHELL_APP_UV_TASK_RUNNER_H_

#include <map>

#include "base/callback.h"
#include "base/single_thread_task_runner.h"
#include "uv.h"  // NOLINT(BUILD/INCLUDE_DIRECTORY)。

namespace base {
class Location;
}

namespace electron {

// 将任务发布到libuv的默认循环的TaskRunner实现。
class UvTaskRunner : public base::SingleThreadTaskRunner {
 public:
  explicit UvTaskRunner(uv_loop_t* loop);

  // Base：：SingleThreadTaskRunner：
  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override;
  bool RunsTasksInCurrentSequence() const override;
  bool PostNonNestableDelayedTask(const base::Location& from_here,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override;

 private:
  ~UvTaskRunner() override;
  static void OnTimeout(uv_timer_t* timer);
  static void OnClose(uv_handle_t* handle);

  uv_loop_t* loop_;

  std::map<uv_timer_t*, base::OnceClosure> tasks_;

  DISALLOW_COPY_AND_ASSIGN(UvTaskRunner);
};

}  // 命名空间电子。

#endif  // Shell_APP_UV_TASK_RUNNER_H_
