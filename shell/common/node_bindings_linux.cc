// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/node_bindings_linux.h"

#include <sys/epoll.h>

namespace electron {

NodeBindingsLinux::NodeBindingsLinux(BrowserEnvironment browser_env)
    : NodeBindings(browser_env), epoll_(epoll_create(1)) {
  int backend_fd = uv_backend_fd(uv_loop_);
  struct epoll_event ev = {0};
  ev.events = EPOLLIN;
  ev.data.fd = backend_fd;
  epoll_ctl(epoll_, EPOLL_CTL_ADD, backend_fd, &ev);
}

NodeBindingsLinux::~NodeBindingsLinux() = default;

void NodeBindingsLinux::RunMessageLoop() {
  // 当libuv的监视器队列更改时得到通知。
  uv_loop_->data = this;
  uv_loop_->on_watcher_queue_updated = OnWatcherQueueChanged;

  NodeBindings::RunMessageLoop();
}

// 静电。
void NodeBindingsLinux::OnWatcherQueueChanged(uv_loop_t* loop) {
  NodeBindingsLinux* self = static_cast<NodeBindingsLinux*>(loop->data);

  // 当循环的监视器出现时，我们需要中断EPOLL线程中的io轮询。
  // 队列更改，否则无法通知新事件。
  self->WakeupEmbedThread();
}

void NodeBindingsLinux::PollEvents() {
  int timeout = uv_backend_timeout(uv_loop_);

  // 等待新的libuv事件。
  int r;
  do {
    struct epoll_event ev;
    r = epoll_wait(epoll_, &ev, 1, timeout);
  } while (r == -1 && errno == EINTR);
}

// 静电。
NodeBindings* NodeBindings::Create(BrowserEnvironment browser_env) {
  return new NodeBindingsLinux(browser_env);
}

}  // 命名空间电子
