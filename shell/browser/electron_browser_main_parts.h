// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_BROWSER_MAIN_PARTS_H_
#define SHELL_BROWSER_ELECTRON_BROWSER_MAIN_PARTS_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/timer/timer.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_main_parts.h"
#include "content/public/common/main_function_params.h"
#include "electron/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/device/public/mojom/geolocation_control.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/views/layout/layout_provider.h"

class BrowserProcessImpl;
class IconManager;

namespace base {
class FieldTrialList;
}

#if defined(USE_AURA)
namespace wm {
class WMState;
}

namespace display {
class Screen;
}
#endif

#if defined(USE_X11)
namespace ui {
class GtkUiPlatform;
}
#endif

namespace device {
class GeolocationManager;
}

namespace electron {

class Browser;
class ElectronBindings;
class JavascriptEnvironment;
class NodeBindings;
class NodeEnvironment;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
class ElectronExtensionsClient;
class ElectronExtensionsBrowserClient;
#endif

#if defined(TOOLKIT_VIEWS)
class ViewsDelegate;
#endif

#if defined(OS_MAC)
class ViewsDelegateMac;
#endif

#if defined(OS_LINUX)
class DarkThemeObserver;
#endif

class ElectronBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit ElectronBrowserMainParts(const content::MainFunctionParams& params);
  ~ElectronBrowserMainParts() override;

  static ElectronBrowserMainParts* Get();

  // 设置退出代码，如果消息循环未准备好，则退出代码将失败。
  bool SetExitCode(int code);

  // 获取退出代码。
  int GetExitCode() const;

  // 返回到GeolocationControl的连接，该连接可以是。
  // 用于为每个客户端启用一次定位服务。
  device::mojom::GeolocationControl* GetGeolocationControl();

#if defined(OS_MAC)
  device::GeolocationManager* GetGeolocationManager();
#endif

  // 将句柄返回给负责提取文件图标的类。
  IconManager* GetIconManager();

  Browser* browser() { return browser_.get(); }
  BrowserProcessImpl* browser_process() { return fake_browser_process_.get(); }

 protected:
  // 内容：：BrowserMainParts：
  int PreEarlyInitialization() override;
  void PostEarlyInitialization() override;
  int PreCreateThreads() override;
  void ToolkitInitialized() override;
  int PreMainMessageLoopRun() override;
  void WillRunMainMessageLoop(
      std::unique_ptr<base::RunLoop>& run_loop) override;
  void PostCreateMainMessageLoop() override;
  void PostMainMessageLoopRun() override;
  void PreCreateMainMessageLoop() override;
  void PostCreateThreads() override;
  void PostDestroyThreads() override;

 private:
  void PreCreateMainMessageLoopCommon();

#if defined(OS_POSIX)
  // 设置信号处理程序。
  void HandleSIGCHLD();
  void InstallShutdownSignalHandlers(
      base::OnceCallback<void()> shutdown_callback,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
#endif

#if defined(OS_MAC)
  void FreeAppDelegate();
  void RegisterURLHandler();
  void InitializeMainNib();
#endif

#if defined(OS_MAC)
  std::unique_ptr<ViewsDelegateMac> views_delegate_;
#else
  std::unique_ptr<ViewsDelegate> views_delegate_;
#endif

#if defined(USE_AURA)
  std::unique_ptr<wm::WMState> wm_state_;
  std::unique_ptr<display::Screen> screen_;
#endif

#if defined(OS_LINUX)
  // 用于通知本机主题更改为暗模式。
  std::unique_ptr<DarkThemeObserver> dark_theme_observer_;
#endif

  std::unique_ptr<views::LayoutProvider> layout_provider_;

  // 用于从Chrome馈送源代码的伪BrowserProcess对象。
  std::unique_ptr<BrowserProcessImpl> fake_browser_process_;

  // 一个在消息循环准备好后记住退出代码的地方。
  // 在此之前，我们只需退出()，而不需要任何中间步骤。
  absl::optional<int> exit_code_;

  std::unique_ptr<JavascriptEnvironment> js_env_;
  std::unique_ptr<Browser> browser_;
  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<ElectronBindings> electron_bindings_;
  std::unique_ptr<NodeEnvironment> node_env_;
  std::unique_ptr<IconManager> icon_manager_;
  std::unique_ptr<base::FieldTrialList> field_trial_list_;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  std::unique_ptr<ElectronExtensionsClient> extensions_client_;
  std::unique_ptr<ElectronExtensionsBrowserClient> extensions_browser_client_;
#endif

  mojo::Remote<device::mojom::GeolocationControl> geolocation_control_;

#if defined(OS_MAC)
  std::unique_ptr<device::GeolocationManager> geolocation_manager_;
#endif

  static ElectronBrowserMainParts* self_;

  DISALLOW_COPY_AND_ASSIGN(ElectronBrowserMainParts);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Electronics_Browser_Main_Parts_H_
