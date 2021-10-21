// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/node_bindings_win.h"

#include <windows.h>

#include "base/logging.h"
#include "base/system/sys_info.h"

namespace electron {

NodeBindingsWin::NodeBindingsWin(BrowserEnvironment browser_env)
    : NodeBindings(browser_env) {
  // 在单核上，io组件端口NumberOfConcurrentThread需要为2。
  // 避免PollEvents中的繁忙循环可能导致的CPU锁定。
  if (base::SysInfo::NumberOfProcessors() == 1) {
    // 预期是uv_loop_刚刚被初始化。
    // 这使得IOCP置换术变得安全。
    CHECK_EQ(0u, uv_loop_->active_handles);
    CHECK_EQ(0u, uv_loop_->active_reqs.count);

    if (uv_loop_->iocp && uv_loop_->iocp != INVALID_HANDLE_VALUE)
      CloseHandle(uv_loop_->iocp);
    uv_loop_->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);
  }
}

NodeBindingsWin::~NodeBindingsWin() = default;

void NodeBindingsWin::PollEvents() {
  // 如果有其他类型的事件挂起，UV_BACKEND_TIMEOUT将。
  // 告诉我们不要再等了。
  DWORD bytes, timeout;
  ULONG_PTR key;
  OVERLAPPED* overlapped;

  timeout = uv_backend_timeout(uv_loop_);

  GetQueuedCompletionStatus(uv_loop_->iocp, &bytes, &key, &overlapped, timeout);

  // 把活动还给我，这样利布就可以处理了。
  if (overlapped != NULL)
    PostQueuedCompletionStatus(uv_loop_->iocp, bytes, key, overlapped);
}

// 静电。
NodeBindings* NodeBindings::Create(BrowserEnvironment browser_env) {
  return new NodeBindingsWin(browser_env);
}

}  // 命名空间电子
