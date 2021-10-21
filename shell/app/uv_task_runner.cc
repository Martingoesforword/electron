// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include <utility>

#include "base/location.h"
#include "base/stl_util.h"
#include "shell/app/uv_task_runner.h"

namespace electron {

UvTaskRunner::UvTaskRunner(uv_loop_t* loop) : loop_(loop) {}

UvTaskRunner::~UvTaskRunner() {
  for (auto& iter : tasks_) {
    uv_unref(reinterpret_cast<uv_handle_t*>(iter.first));
    delete iter.first;
  }
}

bool UvTaskRunner::PostDelayedTask(const base::Location& from_here,
                                   base::OnceClosure task,
                                   base::TimeDelta delay) {
  auto* timer = new uv_timer_t;
  timer->data = this;
  uv_timer_init(loop_, timer);
  uv_timer_start(timer, UvTaskRunner::OnTimeout, delay.InMilliseconds(), 0);
  tasks_[timer] = std::move(task);
  return true;
}

bool UvTaskRunner::RunsTasksInCurrentSequence() const {
  return true;
}

bool UvTaskRunner::PostNonNestableDelayedTask(const base::Location& from_here,
                                              base::OnceClosure task,
                                              base::TimeDelta delay) {
  return PostDelayedTask(from_here, std::move(task), delay);
}

// 静电。
void UvTaskRunner::OnTimeout(uv_timer_t* timer) {
  auto& tasks = static_cast<UvTaskRunner*>(timer->data)->tasks_;
  const auto iter = tasks.find(timer);
  if (iter == std::end(tasks))
    return;

  std::move(iter->second).Run();
  tasks.erase(iter);
  uv_timer_stop(timer);
  uv_close(reinterpret_cast<uv_handle_t*>(timer), UvTaskRunner::OnClose);
}

// 静电。
void UvTaskRunner::OnClose(uv_handle_t* handle) {
  delete reinterpret_cast<uv_timer_t*>(handle);
}

}  // 命名空间电子
