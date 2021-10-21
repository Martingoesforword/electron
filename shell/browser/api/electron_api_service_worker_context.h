// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_
#define SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_

#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/service_worker_context_observer.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"

namespace electron {

class ElectronBrowserContext;

namespace api {

class ServiceWorkerContext
    : public gin::Wrappable<ServiceWorkerContext>,
      public gin_helper::EventEmitterMixin<ServiceWorkerContext>,
      public content::ServiceWorkerContextObserver {
 public:
  static gin::Handle<ServiceWorkerContext> Create(
      v8::Isolate* isolate,
      ElectronBrowserContext* browser_context);

  v8::Local<v8::Value> GetAllRunningWorkerInfo(v8::Isolate* isolate);
  v8::Local<v8::Value> GetWorkerInfoFromID(gin_helper::ErrorThrower thrower,
                                           int64_t version_id);

  // 内容：：ServiceWorkerContextWatch。
  void OnReportConsoleMessage(int64_t version_id,
                              const GURL& scope,
                              const content::ConsoleMessage& message) override;
  void OnRegistrationCompleted(const GURL& scope) override;
  void OnDestruct(content::ServiceWorkerContext* context) override;

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  explicit ServiceWorkerContext(v8::Isolate* isolate,
                                ElectronBrowserContext* browser_context);
  ~ServiceWorkerContext() override;

 private:
  content::ServiceWorkerContext* service_worker_context_;

  base::WeakPtrFactory<ServiceWorkerContext> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerContext);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_SERVICE_WORKER_CONTEXT_H_
