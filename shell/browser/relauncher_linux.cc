// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/relauncher.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>

#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/synchronization/waitable_event.h"

namespace relauncher {

namespace internal {

// 这是全局的，对于sa_handler是可见的。
base::WaitableEvent parentWaiter;

void RelauncherSynchronizeWithParent() {
  base::ScopedFD relauncher_sync_fd(kRelauncherSyncFD);
  static const auto signum = SIGUSR2;

  // 父进程结束时将Signum发送到当前进程。
  if (HANDLE_EINTR(prctl(PR_SET_PDEATHSIG, signum)) != 0) {
    PLOG(ERROR) << "prctl";
    return;
  }

  // 设置Signum处理程序。
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = [](int /* 标牌。*/) { parentWaiter.Signal(); };
  if (sigaction(signum, &action, nullptr) != 0) {
    PLOG(ERROR) << "sigaction";
    return;
  }

  // 将‘\0’字符写入父进程的管道。
  // 这就是家长知道我们已经准备好让它退出的方式。
  if (HANDLE_EINTR(write(relauncher_sync_fd.get(), "", 1)) != 1) {
    PLOG(ERROR) << "write";
    return;
  }

  // 等待父级退出。
  parentWaiter.Wait();
}

int LaunchProgram(const StringVector& relauncher_args,
                  const StringVector& argv) {
  // 将子进程的stdout重定向到/dev/null，否则在。
  // 重新启动子进程将在写入标准输出时引发异常。
  base::ScopedFD devnull(HANDLE_EINTR(open("/dev/null", O_WRONLY)));

  base::LaunchOptions options;
  options.allow_new_privs = true;
  options.new_process_group = true;  // 分离。
  options.fds_to_remap.emplace_back(devnull.get(), STDERR_FILENO);
  options.fds_to_remap.emplace_back(devnull.get(), STDOUT_FILENO);

  base::Process process = base::LaunchProcess(argv, options);
  return process.IsValid() ? 0 : 1;
}

}  // 命名空间内部。

}  // 命名空间重新启动器
