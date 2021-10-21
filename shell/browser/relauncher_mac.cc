// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/relauncher.h"

#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/mac/mac_logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"
#include "base/strings/sys_string_conversions.h"

namespace relauncher {

namespace internal {

void RelauncherSynchronizeWithParent() {
  base::ScopedFD relauncher_sync_fd(kRelauncherSyncFD);

  int parent_pid = getppid();

  // PID 1标识初始化。发射，就是这样。Launchd从不启动。
  // 直接重新启动进程，拥有此PARENT_PID意味着父进程。
  // 已经退出并启动了“继承”重新启动程序作为其子级。
  // 没有理由与Launchd同步。
  if (parent_pid == 1) {
    LOG(ERROR) << "unexpected parent_pid";
    return;
  }

  // 设置kqueue以监视父进程是否退出。
  base::ScopedFD kq(kqueue());
  if (!kq.is_valid()) {
    PLOG(ERROR) << "kqueue";
    return;
  }

  struct kevent change = {0};
  EV_SET(&change, parent_pid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, NULL);
  if (kevent(kq.get(), &change, 1, nullptr, 0, nullptr) == -1) {
    PLOG(ERROR) << "kevent (add)";
    return;
  }

  // 将‘\0’字符写入管道。
  if (HANDLE_EINTR(write(relauncher_sync_fd.get(), "", 1)) != 1) {
    PLOG(ERROR) << "write";
    return;
  }

  // 到目前为止，父进程在读取过程中被阻塞，等待。
  // 请在上面填写以完成。父进程现在可以自由退出。等待。
  // 那将会发生。
  struct kevent event;
  int events = kevent(kq.get(), nullptr, 0, &event, 1, nullptr);
  if (events != 1) {
    if (events < 0) {
      PLOG(ERROR) << "kevent (monitor)";
    } else {
      LOG(ERROR) << "kevent (monitor): unexpected result " << events;
    }
    return;
  }

  if (event.filter != EVFILT_PROC || event.fflags != NOTE_EXIT ||
      event.ident != static_cast<uintptr_t>(parent_pid)) {
    LOG(ERROR) << "kevent (monitor): unexpected event, filter " << event.filter
               << ", fflags " << event.fflags << ", ident " << event.ident;
    return;
  }
}

int LaunchProgram(const StringVector& relauncher_args,
                  const StringVector& argv) {
  // 将子进程的stdout重定向到/dev/null，否则在。
  // 重新启动子进程将在写入标准输出时引发异常。
  base::ScopedFD devnull(HANDLE_EINTR(open("/dev/null", O_WRONLY)));

  base::LaunchOptions options;
  options.new_process_group = true;  // 分离。
  options.fds_to_remap.push_back(std::make_pair(devnull.get(), STDERR_FILENO));
  options.fds_to_remap.push_back(std::make_pair(devnull.get(), STDOUT_FILENO));

  base::Process process = base::LaunchProcess(argv, options);
  return process.IsValid() ? 0 : 1;
}

}  // 命名空间内部。

}  // 命名空间重新启动器
