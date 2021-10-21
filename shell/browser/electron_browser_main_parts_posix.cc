// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

// 大多数代码来自：Chrome/Browser/Chrome_Browser_main_posx.cc。

#include "shell/browser/electron_browser_main_parts.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>

#include "base/debug/leak_annotations.h"
#include "base/posix/eintr_wrapper.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/browser.h"

namespace electron {

namespace {

// 请参阅|PreEarlyInitialization()|中的注释，其中调用sigaction。
void SIGCHLDHandler(int signal) {}

// 在此之前，OSX fork()实现可能会在子进程中崩溃。
// Fork()返回。在这种情况下，关闭的管道仍将是。
// 与父进程共享。要防止儿童撞车，请执行以下操作。
// 导致父进程关闭，|g_PIPE_PID|是进程的PID。
// 它注册了|g_SHUTDOWN_PIPE_WRITE_FD|。
// 请参见&lt;http://crbug.com/175341&gt;.。
pid_t g_pipe_pid = -1;
int g_shutdown_pipe_write_fd = -1;
int g_shutdown_pipe_read_fd = -1;

// SIG{HUP，INT，TERM}处理程序之间的通用代码。
void GracefulShutdownHandler(int signal) {
  // 重新安装默认处理程序。我们只有一次机会优雅地关门。
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIG_DFL;
  RAW_CHECK(sigaction(signal, &action, nullptr) == 0);

  RAW_CHECK(g_pipe_pid == getpid());
  RAW_CHECK(g_shutdown_pipe_write_fd != -1);
  RAW_CHECK(g_shutdown_pipe_read_fd != -1);
  size_t bytes_written = 0;
  do {
    int rv = HANDLE_EINTR(
        write(g_shutdown_pipe_write_fd,
              reinterpret_cast<const char*>(&signal) + bytes_written,
              sizeof(signal) - bytes_written));
    RAW_CHECK(rv >= 0);
    bytes_written += rv;
  } while (bytes_written < sizeof(signal));
}

// 参见|PostCreateMainMessageLoop()|中的注释，其中调用sigaction。
void SIGHUPHandler(int signal) {
  RAW_CHECK(signal == SIGHUP);
  GracefulShutdownHandler(signal);
}

// 参见|PostCreateMainMessageLoop()|中的注释，其中调用sigaction。
void SIGINTHandler(int signal) {
  RAW_CHECK(signal == SIGINT);
  GracefulShutdownHandler(signal);
}

// 参见|PostCreateMainMessageLoop()|中的注释，其中调用sigaction。
void SIGTERMHandler(int signal) {
  RAW_CHECK(signal == SIGTERM);
  GracefulShutdownHandler(signal);
}

class ShutdownDetector : public base::PlatformThread::Delegate {
 public:
  explicit ShutdownDetector(
      int shutdown_fd,
      base::OnceCallback<void()> shutdown_callback,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);

  // Base：：PlatformThread：：Delegate：
  void ThreadMain() override;

 private:
  const int shutdown_fd_;
  base::OnceCallback<void()> shutdown_callback_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ShutdownDetector);
};

ShutdownDetector::ShutdownDetector(
    int shutdown_fd,
    base::OnceCallback<void()> shutdown_callback,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : shutdown_fd_(shutdown_fd),
      shutdown_callback_(std::move(shutdown_callback)),
      task_runner_(task_runner) {
  CHECK_NE(shutdown_fd_, -1);
  CHECK(!shutdown_callback_.is_null());
  CHECK(task_runner_);
}

// 这些函数用于帮助我们诊断发生的崩溃转储。
// 在关闭过程中。
NOINLINE void ShutdownFDReadError() {
  // 确保功能不会被优化。
  asm("");
  sleep(UINT_MAX);
}

NOINLINE void ShutdownFDClosedError() {
  // 确保功能不会被优化。
  asm("");
  sleep(UINT_MAX);
}

NOINLINE void ExitPosted() {
  // 确保功能不会被优化。
  asm("");
  sleep(UINT_MAX);
}

void ShutdownDetector::ThreadMain() {
  base::PlatformThread::SetName("CrShutdownDetector");

  int signal;
  size_t bytes_read = 0;
  do {
    const ssize_t ret = HANDLE_EINTR(
        read(shutdown_fd_, reinterpret_cast<char*>(&signal) + bytes_read,
             sizeof(signal) - bytes_read));
    if (ret < 0) {
      NOTREACHED() << "Unexpected error: " << strerror(errno);
      ShutdownFDReadError();
      break;
    } else if (ret == 0) {
      NOTREACHED() << "Unexpected closure of shutdown pipe.";
      ShutdownFDClosedError();
      break;
    }
    bytes_read += ret;
  } while (bytes_read < sizeof(signal));
  VLOG(1) << "Handling shutdown for signal " << signal << ".";

  if (!task_runner_->PostTask(FROM_HERE,
                              base::BindOnce(std::move(shutdown_callback_)))) {
    // 如果没有有效的任务运行者来发布退出任务，就不会有太多的任务运行者。
    // 选择。再次发出信号。默认处理程序将获取它。
    // 导致一个不体面的退场。
    RAW_LOG(WARNING, "No valid task runner, exiting ungracefully.");
    kill(getpid(), signal);

    // 该信号可以在另一个线程上处理。给他一个机会。
    // 会发生的。
    sleep(3);

    // 我们现在真的应该已经死了。不管出于什么原因，我们都不是。出口。
    // 立即，将退出状态设置为具有位8的信号号。
    // 准备好了。在我们关心的系统上，此退出状态是。
    // 通常用于指示此信号的默认处理程序退出。
    // 这种机制不是法律上的标准，但即使在最坏的情况下，它也。
    // 至少应该会导致立即退出。
    RAW_LOG(WARNING, "Still here, exiting really ungracefully.");
    _exit(signal | (1 << 7));
  }
  ExitPosted();
}

}  // 命名空间。

void ElectronBrowserMainParts::HandleSIGCHLD() {
  // 我们需要接受SIGCHLD，即使我们的处理程序是无操作的，因为。
  // 否则我们就不能伺候孩子了。(根据POSIX2001。)。
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIGCHLDHandler;
  CHECK_EQ(sigaction(SIGCHLD, &action, nullptr), 0);
}

void ElectronBrowserMainParts::InstallShutdownSignalHandlers(
    base::OnceCallback<void()> shutdown_callback,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  int pipefd[2];
  int ret = pipe(pipefd);
  if (ret < 0) {
    PLOG(DFATAL) << "Failed to create pipe";
    return;
  }
  g_pipe_pid = getpid();
  g_shutdown_pipe_read_fd = pipefd[0];
  g_shutdown_pipe_write_fd = pipefd[1];
#if !defined(ADDRESS_SANITIZER)
  const size_t kShutdownDetectorThreadStackSize = PTHREAD_STACK_MIN * 2;
#else
  // ASAN检测会使堆栈帧膨胀，因此我们需要增加。
  // 堆栈大小以避免命中保护页。
  const size_t kShutdownDetectorThreadStackSize = PTHREAD_STACK_MIN * 4;
#endif
  ShutdownDetector* detector = new ShutdownDetector(
      g_shutdown_pipe_read_fd, std::move(shutdown_callback), task_runner);

  // PlatformThread不删除其委托。
  ANNOTATE_LEAKING_OBJECT_PTR(detector);
  if (!base::PlatformThread::CreateNonJoinable(kShutdownDetectorThreadStackSize,
                                               detector)) {
    LOG(DFATAL) << "Failed to create shutdown detector task.";
  }
  // 设置关闭管道后用于关闭的设置信号处理程序，因为。
  // 可以在设置处理程序后立即调用它。

  // 如果添加到此信号处理程序列表中，请注意新信号可能。
  // 需要在子进程中重置。看见。
  // Base/process_util_position x.cc：LaunchProcess。

  // 我们需要处理SIGTERM，因为这就是多少基于POSIX的发行版。
  // 要求进程在关闭时正常退出。
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = SIGTERMHandler;
  CHECK_EQ(sigaction(SIGTERM, &action, nullptr), 0);

  // 还可以处理SIGINT-当用户通过Ctrl+C终止浏览器时。如果。
  // 正在调试浏览器进程，gdb将首先捕获SIGINT。
  action.sa_handler = SIGINTHandler;
  CHECK_EQ(sigaction(SIGINT, &action, nullptr), 0);

  // 和SIGHUP，用于终端消失时。在关机时，许多Linux。
  // 发行版发送SIGHUP、SIGTERM，然后发送SIGKILL。
  action.sa_handler = SIGHUPHandler;
  CHECK_EQ(sigaction(SIGHUP, &action, nullptr), 0);
}

}  // 命名空间电子
