// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_LIB_POWER_OBSERVER_LINUX_H_
#define SHELL_BROWSER_LIB_POWER_OBSERVER_LINUX_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/power_monitor/power_observer.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

namespace electron {

class PowerObserverLinux {
 public:
  explicit PowerObserverLinux(base::PowerSuspendObserver* suspend_observer);
  ~PowerObserverLinux();

  void SetShutdownHandler(base::RepeatingCallback<bool()> should_shutdown);

 private:
  void BlockSleep();
  void UnblockSleep();
  void BlockShutdown();
  void UnblockShutdown();

  void OnLoginServiceAvailable(bool available);
  void OnInhibitResponse(base::ScopedFD* scoped_fd, dbus::Response* response);
  void OnPrepareForSleep(dbus::Signal* signal);
  void OnPrepareForShutdown(dbus::Signal* signal);
  void OnSignalConnected(const std::string& interface,
                         const std::string& signal,
                         bool success);

  base::RepeatingCallback<bool()> should_shutdown_;
  base::PowerSuspendObserver* suspend_observer_ = nullptr;

  scoped_refptr<dbus::ObjectProxy> logind_;
  std::string lock_owner_name_;
  base::ScopedFD sleep_lock_;
  base::ScopedFD shutdown_lock_;
  base::WeakPtrFactory<PowerObserverLinux> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(PowerObserverLinux);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Lib_Power_Observator_Linux_H_
