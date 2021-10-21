// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/node_bindings_mac.h"

#include <errno.h>
#include <sys/select.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include "shell/common/node_includes.h"

namespace electron {

NodeBindingsMac::NodeBindingsMac(BrowserEnvironment browser_env)
    : NodeBindings(browser_env) {}

NodeBindingsMac::~NodeBindingsMac() = default;

void NodeBindingsMac::RunMessageLoop() {
  // 当libuv的监视器队列更改时得到通知。
  uv_loop_->data = this;
  uv_loop_->on_watcher_queue_updated = OnWatcherQueueChanged;

  NodeBindings::RunMessageLoop();
}

// 静电。
void NodeBindingsMac::OnWatcherQueueChanged(uv_loop_t* loop) {
  NodeBindingsMac* self = static_cast<NodeBindingsMac*>(loop->data);

  // 当循环的监视器出现时，我们需要中断kqueue线程中的io轮询。
  // 队列更改，否则无法通知新事件。
  self->WakeupEmbedThread();
}

void NodeBindingsMac::PollEvents() {
  struct timeval tv;
  int timeout = uv_backend_timeout(uv_loop_);
  if (timeout != -1) {
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
  }

  fd_set readset;
  int fd = uv_backend_fd(uv_loop_);
  FD_ZERO(&readset);
  FD_SET(fd, &readset);

  // 等待新的libuv事件。
  int r;
  do {
    r = select(fd + 1, &readset, nullptr, nullptr,
               timeout == -1 ? nullptr : &tv);
  } while (r == -1 && errno == EINTR);
}

// 静电。
NodeBindings* NodeBindings::Create(BrowserEnvironment browser_env) {
  return new NodeBindingsMac(browser_env);
}

}  // 命名空间电子
