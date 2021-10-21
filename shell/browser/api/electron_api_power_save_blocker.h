// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_POWER_SAVE_BLOCKER_H_
#define SHELL_BROWSER_API_ELECTRON_API_POWER_SAVE_BLOCKER_H_

#include <map>

#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/device/public/mojom/wake_lock.mojom.h"

namespace electron {

namespace api {

class PowerSaveBlocker : public gin::Wrappable<PowerSaveBlocker> {
 public:
  static gin::Handle<PowerSaveBlocker> Create(v8::Isolate* isolate);

  // 杜松子酒：：可包装的。
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  static gin::WrapperInfo kWrapperInfo;

 protected:
  explicit PowerSaveBlocker(v8::Isolate* isolate);
  ~PowerSaveBlocker() override;

 private:
  void UpdatePowerSaveBlocker();
  int Start(device::mojom::WakeLockType type);
  bool Stop(int id);
  bool IsStarted(int id);

  device::mojom::WakeLock* GetWakeLock();

  // 当前唤醒锁定级别。
  device::mojom::WakeLockType current_lock_type_;

  // 唤醒锁当前是否处于活动状态。
  bool is_wake_lock_active_ = false;

  // 将id映射到每个请求的相应拦截器类型。
  using WakeLockTypeMap = std::map<int, device::mojom::WakeLockType>;
  WakeLockTypeMap wake_lock_types_;

  mojo::Remote<device::mojom::WakeLock> wake_lock_;

  DISALLOW_COPY_AND_ASSIGN(PowerSaveBlocker);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_POWER_SAVE_BLOCKER_H_
