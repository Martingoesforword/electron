// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_POWER_MONITOR_H_
#define SHELL_BROWSER_API_ELECTRON_API_POWER_MONITOR_H_

#include "base/power_monitor/power_observer.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/pinnable.h"
#include "ui/base/idle/idle.h"

#if defined(OS_LINUX)
#include "shell/browser/lib/power_observer_linux.h"
#endif

namespace electron {

namespace api {

class PowerMonitor : public gin::Wrappable<PowerMonitor>,
                     public gin_helper::EventEmitterMixin<PowerMonitor>,
                     public gin_helper::Pinnable<PowerMonitor>,
                     public base::PowerStateObserver,
                     public base::PowerSuspendObserver {
 public:
  static v8::Local<v8::Value> Create(v8::Isolate* isolate);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 private:
  explicit PowerMonitor(v8::Isolate* isolate);
  ~PowerMonitor() override;

#if defined(OS_LINUX)
  void SetListeningForShutdown(bool);
#endif

  // 由本地调用调用。
  bool ShouldShutdown();

#if defined(OS_MAC) || defined(OS_WIN)
  void InitPlatformSpecificMonitors();
#endif

  // Base：：PowerStateViewer实现：
  void OnPowerStateChange(bool on_battery_power) override;

  // Base：：PowerSuspendWatch实现：
  void OnSuspend() override;
  void OnResume() override;

#if defined(OS_WIN)
  // 当消息进入我们的消息传递窗口时调用静态回调。
  static LRESULT CALLBACK WndProcStatic(HWND hwnd,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam);

  LRESULT CALLBACK WndProc(HWND hwnd,
                           UINT message,
                           WPARAM wparam,
                           LPARAM lparam);

  // |Window_|的窗口类。
  ATOM atom_;

  // 包含|Window_|的窗口过程的模块的句柄。
  HMODULE instance_;

  // 用于处理事件的窗口。
  HWND window_;
#endif

#if defined(OS_LINUX)
  PowerObserverLinux power_observer_linux_{this};
#endif

  DISALLOW_COPY_AND_ASSIGN(PowerMonitor);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_POWER_MONITOR_H_
