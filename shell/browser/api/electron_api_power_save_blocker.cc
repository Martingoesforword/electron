// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_power_save_blocker.h"

#include <string>

#include "base/callback_helpers.h"
#include "base/task/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/device_service.h"
#include "gin/dictionary.h"
#include "gin/function_template.h"
#include "services/device/public/mojom/wake_lock_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "shell/common/node_includes.h"

namespace gin {

template <>
struct Converter<device::mojom::WakeLockType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     device::mojom::WakeLockType* out) {
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "prevent-app-suspension")
      *out = device::mojom::WakeLockType::kPreventAppSuspension;
    else if (type == "prevent-display-sleep")
      *out = device::mojom::WakeLockType::kPreventDisplaySleep;
    else
      return false;
    return true;
  }
};

}  // 命名空间杜松子酒。

namespace electron {

namespace api {

gin::WrapperInfo PowerSaveBlocker::kWrapperInfo = {gin::kEmbedderNativeGin};

PowerSaveBlocker::PowerSaveBlocker(v8::Isolate* isolate)
    : current_lock_type_(device::mojom::WakeLockType::kPreventAppSuspension) {}

PowerSaveBlocker::~PowerSaveBlocker() = default;

void PowerSaveBlocker::UpdatePowerSaveBlocker() {
  if (wake_lock_types_.empty()) {
    if (is_wake_lock_active_) {
      GetWakeLock()->CancelWakeLock();
      is_wake_lock_active_ = false;
    }
    return;
  }

  // |WakeLockType：：kPreventAppSusending|保持系统活动，但允许。
  // 要关闭的屏幕。
  // |WakeLockType：：kPreventDisplaySept|保持系统和屏幕处于活动状态，有。
  // 优先级高于|WakeLockType：：kPreventAppSusounding|。
  // 
  // 只有优先级最高的拦截器类型才有效。
  device::mojom::WakeLockType new_lock_type =
      device::mojom::WakeLockType::kPreventAppSuspension;
  for (const auto& element : wake_lock_types_) {
    if (element.second == device::mojom::WakeLockType::kPreventDisplaySleep) {
      new_lock_type = device::mojom::WakeLockType::kPreventDisplaySleep;
      break;
    }
  }

  if (current_lock_type_ != new_lock_type) {
    GetWakeLock()->ChangeType(new_lock_type, base::DoNothing());
    current_lock_type_ = new_lock_type;
  }
  if (!is_wake_lock_active_) {
    GetWakeLock()->RequestWakeLock();
    is_wake_lock_active_ = true;
  }
}

device::mojom::WakeLock* PowerSaveBlocker::GetWakeLock() {
  if (!wake_lock_) {
    mojo::Remote<device::mojom::WakeLockProvider> wake_lock_provider;
    content::GetDeviceService().BindWakeLockProvider(
        wake_lock_provider.BindNewPipeAndPassReceiver());

    wake_lock_provider->GetWakeLockWithoutContext(
        device::mojom::WakeLockType::kPreventAppSuspension,
        device::mojom::WakeLockReason::kOther, ELECTRON_PRODUCT_NAME,
        wake_lock_.BindNewPipeAndPassReceiver());
  }
  return wake_lock_.get();
}

int PowerSaveBlocker::Start(device::mojom::WakeLockType type) {
  static int count = 0;
  wake_lock_types_[count] = type;
  UpdatePowerSaveBlocker();
  return count++;
}

bool PowerSaveBlocker::Stop(int id) {
  bool success = wake_lock_types_.erase(id) > 0;
  UpdatePowerSaveBlocker();
  return success;
}

bool PowerSaveBlocker::IsStarted(int id) {
  return wake_lock_types_.find(id) != wake_lock_types_.end();
}

// 静电。
gin::Handle<PowerSaveBlocker> PowerSaveBlocker::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new PowerSaveBlocker(isolate));
}

gin::ObjectTemplateBuilder PowerSaveBlocker::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<PowerSaveBlocker>::GetObjectTemplateBuilder(isolate)
      .SetMethod("start", &PowerSaveBlocker::Start)
      .SetMethod("stop", &PowerSaveBlocker::Stop)
      .SetMethod("isStarted", &PowerSaveBlocker::IsStarted);
}

}  // 命名空间API。

}  // 命名空间电子。

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin::Dictionary dict(isolate, exports);
  dict.Set("powerSaveBlocker",
           electron::api::PowerSaveBlocker::Create(isolate));
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_power_save_blocker,
                                 Initialize)
