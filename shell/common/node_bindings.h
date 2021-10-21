// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_NODE_BINDINGS_H_
#define SHELL_COMMON_NODE_BINDINGS_H_

#include <type_traits>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "uv.h"  // NOLINT(BUILD/INCLUDE_DIRECTORY)。
#include "v8/include/v8.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace node {
class Environment;
class MultiIsolatePlatform;
class IsolateData;
}  // 命名空间节点。

namespace electron {

// 管理uv_Handle_t类型的帮助器类，例如uv_async_t。
// 
// 根据UV文档：“必须在每个句柄上调用UV_Close()，然后才能。
// 释放内存。此外，内存只能在。
// CLOSE_CB或在其返回之后。“此类封装工作。
// 需要遵循这些要求。
template <typename T,
          typename std::enable_if<
              // 这些是uv_Handle_t的C样式“子类”
              std::is_same<T, uv_async_t>::value ||
              std::is_same<T, uv_check_t>::value ||
              std::is_same<T, uv_fs_event_t>::value ||
              std::is_same<T, uv_fs_poll_t>::value ||
              std::is_same<T, uv_idle_t>::value ||
              std::is_same<T, uv_pipe_t>::value ||
              std::is_same<T, uv_poll_t>::value ||
              std::is_same<T, uv_prepare_t>::value ||
              std::is_same<T, uv_process_t>::value ||
              std::is_same<T, uv_signal_t>::value ||
              std::is_same<T, uv_stream_t>::value ||
              std::is_same<T, uv_tcp_t>::value ||
              std::is_same<T, uv_timer_t>::value ||
              std::is_same<T, uv_tty_t>::value ||
              std::is_same<T, uv_udp_t>::value>::type* = nullptr>
class UvHandle {
 public:
  UvHandle() : t_(new T) {}
  ~UvHandle() { reset(); }
  T* get() { return t_; }
  uv_handle_t* handle() { return reinterpret_cast<uv_handle_t*>(t_); }

  void reset() {
    auto* h = handle();
    if (h != nullptr) {
      DCHECK_EQ(0, uv_is_closing(h));
      uv_close(h, OnClosed);
      t_ = nullptr;
    }
  }

 private:
  static void OnClosed(uv_handle_t* handle) {
    delete reinterpret_cast<T*>(handle);
  }

  T* t_ = {};
};

class NodeBindings {
 public:
  enum class BrowserEnvironment { kBrowser, kRenderer, kWorker };

  static NodeBindings* Create(BrowserEnvironment browser_env);
  static void RegisterBuiltinModules();
  static bool IsInitialized();

  virtual ~NodeBindings();

  // 设置V8，libuv。
  void Initialize();

  // 创建环境并加载node.js。
  node::Environment* CreateEnvironment(v8::Handle<v8::Context> context,
                                       node::MultiIsolatePlatform* platform);

  // 在环境中加载node.js。
  void LoadEnvironment(node::Environment* env);

  // 为消息循环集成做好准备。
  void PrepareMessageLoop();

  // 进行消息循环集成。
  virtual void RunMessageLoop();

  node::IsolateData* isolate_data() const { return isolate_data_; }

  // 获取/设置要包裹UV循环的环境。
  void set_uv_env(node::Environment* env) { uv_env_ = env; }
  node::Environment* uv_env() const { return uv_env_; }

  uv_loop_t* uv_loop() const { return uv_loop_; }

  bool in_worker_loop() const { return uv_loop_ == &worker_loop_; }

 protected:
  explicit NodeBindings(BrowserEnvironment browser_env);

  // 调用以轮询新线程中的事件。
  virtual void PollEvents() = 0;

  // 运行一次libuv循环。
  void UvRunOnce();

  // 让主线程运行libuv循环。
  void WakeupMainThread();

  // 中断PollEvents。
  void WakeupEmbedThread();

  // 我们正在运行的环境。
  const BrowserEnvironment browser_env_;

  // 当前线程的MessageLoop。
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // 当前线程的libuv循环。
  uv_loop_t* uv_loop_;

 private:
  // 用于轮询UV事件的线程。
  static void EmbedThreadRunner(void* arg);

  // Libuv循环是否已经结束。
  bool embed_closed_ = false;

  // 在工作模式下构建时使用的循环。
  uv_loop_t worker_loop_;

  // 虚拟手柄，使UV的循环不退出。
  UvHandle<uv_async_t> dummy_uv_handle_;

  // 轮询事件的线程。
  uv_thread_t embed_thread_;

  // 等待嵌入线程中的主循环的信号量。
  uv_sem_t embed_sem_;

  // 包裹UV循环的环境。
  node::Environment* uv_env_ = nullptr;

  // 隔离在创建环境中使用的数据。
  node::IsolateData* isolate_data_ = nullptr;

#if !defined(OS_WIN)
  int handle_ = -1;
#endif

  base::WeakPtrFactory<NodeBindings> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(NodeBindings);
};

}  // 命名空间电子。

#endif  // Shell_COMMON_NODE_BINDINGS_H_
