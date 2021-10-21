// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_AUTO_UPDATER_H_
#define SHELL_BROWSER_API_ELECTRON_API_AUTO_UPDATER_H_

#include <string>

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/auto_updater.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/window_list_observer.h"

namespace electron {

namespace api {

class AutoUpdater : public gin::Wrappable<AutoUpdater>,
                    public gin_helper::EventEmitterMixin<AutoUpdater>,
                    public auto_updater::Delegate,
                    public WindowListObserver {
 public:
  static gin::Handle<AutoUpdater> Create(v8::Isolate* isolate);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  AutoUpdater();
  ~AutoUpdater() override;

  // 委托实现。
  void OnError(const std::string& message) override;
  void OnError(const std::string& message,
               const int code,
               const std::string& domain) override;
  void OnCheckingForUpdate() override;
  void OnUpdateAvailable() override;
  void OnUpdateNotAvailable() override;
  void OnUpdateDownloaded(const std::string& release_notes,
                          const std::string& release_name,
                          const base::Time& release_date,
                          const std::string& update_url) override;

  // WindowListViewer：
  void OnWindowAllClosed() override;

 private:
  std::string GetFeedURL();
  void SetFeedURL(gin::Arguments* args);
  void QuitAndInstall();

  DISALLOW_COPY_AND_ASSIGN(AutoUpdater);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_AUTO_UPDATER_H_
