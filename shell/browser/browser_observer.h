// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_BROWSER_OBSERVER_H_
#define SHELL_BROWSER_BROWSER_OBSERVER_H_

#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/observer_list_types.h"
#include "build/build_config.h"
#include "shell/browser/login_handler.h"

namespace base {
class DictionaryValue;
}

namespace electron {

class BrowserObserver : public base::CheckedObserver {
 public:
  // 浏览器即将关闭所有窗口。
  virtual void OnBeforeQuit(bool* prevent_default) {}

  // 浏览器已关闭所有窗口，并将退出。
  virtual void OnWillQuit(bool* prevent_default) {}

  // 浏览器已关闭所有窗口。如果浏览器正在退出，则此。
  // 方法，而是调用OnWillQuit。
  virtual void OnWindowAllClosed() {}

  // 浏览器正在退出。
  virtual void OnQuit() {}

  // 浏览器已通过在Finder中双击或拖动。
  // 文件添加到Dock图标。(仅限MacOS)。
  virtual void OnOpenFile(bool* prevent_default, const std::string& file_path) {
  }

  // 浏览器用于打开URL。
  virtual void OnOpenURL(const std::string& url) {}

  // 浏览器通过可见/不可见窗口激活(通常通过。
  // 点击停靠图标)。
  virtual void OnActivate(bool has_visible_windows) {}

  // 浏览器已完成加载。
  virtual void OnWillFinishLaunching() {}
  virtual void OnFinishLaunching(const base::DictionaryValue& launch_info) {}

  // 浏览器的辅助功能支持已更改。
  virtual void OnAccessibilitySupportChanged() {}

  // 应用程序消息循环已就绪。
  virtual void OnPreMainMessageLoopRun() {}

  // 在创建应用程序线程之前调用，这是第一次访问的位置。
  // 应创建进程内GpuDataManager。
  // 请参阅https://chromium-review.googlesource.com/c/chromium/src/+/2134864。
  virtual void OnPreCreateThreads() {}

#if defined(OS_MAC)
  // 浏览器想要报告用户活动将恢复。(仅限MacOS)。
  virtual void OnWillContinueUserActivity(bool* prevent_default,
                                          const std::string& type) {}
  // 浏览器要报告用户活动恢复错误。(仅限MacOS)。
  virtual void OnDidFailToContinueUserActivity(const std::string& type,
                                               const std::string& error) {}
  // 浏览器希望通过切换恢复用户活动。(仅限MacOS)。
  virtual void OnContinueUserActivity(bool* prevent_default,
                                      const std::string& type,
                                      const base::DictionaryValue& user_info,
                                      const base::DictionaryValue& details) {}
  // 浏览器想要通知用户活动已恢复。(仅限MacOS)。
  virtual void OnUserActivityWasContinued(
      const std::string& type,
      const base::DictionaryValue& user_info) {}
  // 浏览器想要更新用户活动有效负载。(仅限MacOS)。
  virtual void OnUpdateUserActivityState(
      bool* prevent_default,
      const std::string& type,
      const base::DictionaryValue& user_info) {}
  // 用户点击了原生的MacOS新建选项卡按钮。(仅限MacOS)。
  virtual void OnNewWindowForTab() {}

  // 浏览器确实处于活动状态。
  virtual void OnDidBecomeActive() {}
#endif

 protected:
  ~BrowserObserver() override {}
};

}  // 命名空间电子。

#endif  // 外壳浏览器浏览器观察者H_
