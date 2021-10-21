// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。
#include "shell/browser/lib/power_observer_linux.h"

#include <unistd.h>
#include <uv.h>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "device/bluetooth/dbus/bluez_dbus_thread_manager.h"

namespace {

const char kLogindServiceName[] = "org.freedesktop.login1";
const char kLogindObjectPath[] = "/org/freedesktop/login1";
const char kLogindManagerInterface[] = "org.freedesktop.login1.Manager";

base::FilePath::StringType GetExecutableBaseName() {
  return base::CommandLine::ForCurrentProcess()
      ->GetProgram()
      .BaseName()
      .value();
}

}  // 命名空间。

namespace electron {

PowerObserverLinux::PowerObserverLinux(
    base::PowerSuspendObserver* suspend_observer)
    : suspend_observer_(suspend_observer),
      lock_owner_name_(GetExecutableBaseName()) {
  auto* bus = bluez::BluezDBusThreadManager::Get()->GetSystemBus();
  if (!bus) {
    LOG(WARNING) << "Failed to get system bus connection";
    return;
  }

  // 设置登录代理。

  const auto weakThis = weak_ptr_factory_.GetWeakPtr();

  logind_ = bus->GetObjectProxy(kLogindServiceName,
                                dbus::ObjectPath(kLogindObjectPath));
  logind_->ConnectToSignal(
      kLogindManagerInterface, "PrepareForShutdown",
      base::BindRepeating(&PowerObserverLinux::OnPrepareForShutdown, weakThis),
      base::BindRepeating(&PowerObserverLinux::OnSignalConnected, weakThis));
  logind_->ConnectToSignal(
      kLogindManagerInterface, "PrepareForSleep",
      base::BindRepeating(&PowerObserverLinux::OnPrepareForSleep, weakThis),
      base::BindRepeating(&PowerObserverLinux::OnSignalConnected, weakThis));
  logind_->WaitForServiceToBeAvailable(base::BindRepeating(
      &PowerObserverLinux::OnLoginServiceAvailable, weakThis));
}

PowerObserverLinux::~PowerObserverLinux() = default;

void PowerObserverLinux::OnLoginServiceAvailable(bool service_available) {
  if (!service_available) {
    LOG(WARNING) << kLogindServiceName << " not available";
    return;
  }
  // 采取睡眠抑制锁。
  BlockSleep();
}

void PowerObserverLinux::BlockSleep() {
  dbus::MethodCall sleep_inhibit_call(kLogindManagerInterface, "Inhibit");
  dbus::MessageWriter inhibit_writer(&sleep_inhibit_call);
  inhibit_writer.AppendString("sleep");  // 什么。
  // 使用可执行文件名称作为锁所有者，这将列出。
  // 电子可作为独立实体执行。
  inhibit_writer.AppendString(lock_owner_name_);                      // 谁。
  inhibit_writer.AppendString("Application cleanup before suspend");  // 为什么。
  inhibit_writer.AppendString("delay");                               // 模式。
  logind_->CallMethod(
      &sleep_inhibit_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::BindOnce(&PowerObserverLinux::OnInhibitResponse,
                     weak_ptr_factory_.GetWeakPtr(), &sleep_lock_));
}

void PowerObserverLinux::UnblockSleep() {
  sleep_lock_.reset();
}

void PowerObserverLinux::BlockShutdown() {
  if (shutdown_lock_.is_valid()) {
    LOG(WARNING) << "Trying to subscribe to shutdown multiple times";
    return;
  }
  dbus::MethodCall shutdown_inhibit_call(kLogindManagerInterface, "Inhibit");
  dbus::MessageWriter inhibit_writer(&shutdown_inhibit_call);
  inhibit_writer.AppendString("shutdown");                 // 什么。
  inhibit_writer.AppendString(lock_owner_name_);           // 谁。
  inhibit_writer.AppendString("Ensure a clean shutdown");  // 为什么。
  inhibit_writer.AppendString("delay");                    // 模式。
  logind_->CallMethod(
      &shutdown_inhibit_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::BindOnce(&PowerObserverLinux::OnInhibitResponse,
                     weak_ptr_factory_.GetWeakPtr(), &shutdown_lock_));
}

void PowerObserverLinux::UnblockShutdown() {
  if (!shutdown_lock_.is_valid()) {
    LOG(WARNING)
        << "Trying to unsubscribe to shutdown without being subscribed";
    return;
  }
  shutdown_lock_.reset();
}

void PowerObserverLinux::SetShutdownHandler(
    base::RepeatingCallback<bool()> handler) {
  // 为了在调用e.PrevenentDefault()时延迟系统关机。
  // 在powerMonitor‘Shutdown’事件上，我们需要一个org.freedesktop.login1。
  // 停机延迟锁定。有关更多详细信息，请参阅“采用延迟锁”(Take Delay Lock)。
  // Https://www.freedesktop.org/wiki/Software/systemd/inhibit/部分。
  if (handler && !should_shutdown_) {
    BlockShutdown();
  } else if (!handler && should_shutdown_) {
    UnblockShutdown();
  }
  should_shutdown_ = std::move(handler);
}

void PowerObserverLinux::OnInhibitResponse(base::ScopedFD* scoped_fd,
                                           dbus::Response* response) {
  if (response != nullptr) {
    dbus::MessageReader reader(response);
    reader.PopFileDescriptor(scoped_fd);
  }
}

void PowerObserverLinux::OnPrepareForSleep(dbus::Signal* signal) {
  dbus::MessageReader reader(signal);
  bool suspending;
  if (!reader.PopBool(&suspending)) {
    LOG(ERROR) << "Invalid signal: " << signal->ToString();
    return;
  }
  if (suspending) {
    suspend_observer_->OnSuspend();

    UnblockSleep();
  } else {
    BlockSleep();

    suspend_observer_->OnResume();
  }
}

void PowerObserverLinux::OnPrepareForShutdown(dbus::Signal* signal) {
  dbus::MessageReader reader(signal);
  bool shutting_down;
  if (!reader.PopBool(&shutting_down)) {
    LOG(ERROR) << "Invalid signal: " << signal->ToString();
    return;
  }
  if (shutting_down) {
    if (!should_shutdown_ || should_shutdown_.Run()) {
      // 用户未尝试阻止关机。释放锁并允许。
      // 关闭以正常继续。
      shutdown_lock_.reset();
    }
  }
}

void PowerObserverLinux::OnSignalConnected(const std::string& /* 接口。*/,
                                           const std::string& signal,
                                           bool success) {
  LOG_IF(WARNING, !success) << "Failed to connect to " << signal;
}

}  // 命名空间电子
