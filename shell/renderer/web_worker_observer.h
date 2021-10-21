// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_WEB_WORKER_OBSERVER_H_
#define SHELL_RENDERER_WEB_WORKER_OBSERVER_H_

#include <memory>

#include "base/macros.h"
#include "v8/include/v8.h"

namespace electron {

class ElectronBindings;
class NodeBindings;

// 监视WebWorker并向其插入节点集成。
class WebWorkerObserver {
 public:
  // 返回当前工作线程的WebWorkerWatch。
  static WebWorkerObserver* GetCurrent();

  void WorkerScriptReadyForEvaluation(v8::Local<v8::Context> context);
  void ContextWillDestroy(v8::Local<v8::Context> context);

 private:
  WebWorkerObserver();
  ~WebWorkerObserver();

  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<ElectronBindings> electron_bindings_;

  DISALLOW_COPY_AND_ASSIGN(WebWorkerObserver);
};

}  // 命名空间电子。

#endif  // Shell_渲染器_Web_Worker_观察者_H_
