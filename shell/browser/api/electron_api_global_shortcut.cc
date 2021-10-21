// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_global_shortcut.h"

#include <vector>

#include "base/containers/contains.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/electron_api_system_preferences.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_converters/accelerator_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/node_includes.h"

#if defined(OS_MAC)
#include "base/mac/mac_util.h"
#endif

using extensions::GlobalShortcutListener;

namespace {

#if defined(OS_MAC)
bool RegisteringMediaKeyForUntrustedClient(const ui::Accelerator& accelerator) {
  if (base::mac::IsAtLeastOS10_14()) {
    constexpr ui::KeyboardCode mediaKeys[] = {
        ui::VKEY_MEDIA_PLAY_PAUSE, ui::VKEY_MEDIA_NEXT_TRACK,
        ui::VKEY_MEDIA_PREV_TRACK, ui::VKEY_MEDIA_STOP,
        ui::VKEY_VOLUME_UP,        ui::VKEY_VOLUME_DOWN,
        ui::VKEY_VOLUME_MUTE};

    if (std::find(std::begin(mediaKeys), std::end(mediaKeys),
                  accelerator.key_code()) != std::end(mediaKeys)) {
      bool trusted =
          electron::api::SystemPreferences::IsTrustedAccessibilityClient(false);
      if (!trusted)
        return true;
    }
  }
  return false;
}
#endif

}  // 命名空间。

namespace electron {

namespace api {

gin::WrapperInfo GlobalShortcut::kWrapperInfo = {gin::kEmbedderNativeGin};

GlobalShortcut::GlobalShortcut(v8::Isolate* isolate) {}

GlobalShortcut::~GlobalShortcut() {
  UnregisterAll();
}

void GlobalShortcut::OnKeyPressed(const ui::Accelerator& accelerator) {
  if (accelerator_callback_map_.find(accelerator) ==
      accelerator_callback_map_.end()) {
    // 这应该永远不会发生，因为如果发生这种情况，GlobalShortcutListener。
    // 用错误的加速器通知我们。
    NOTREACHED();
    return;
  }
  accelerator_callback_map_[accelerator].Run();
}

bool GlobalShortcut::RegisterAll(
    const std::vector<ui::Accelerator>& accelerators,
    const base::RepeatingClosure& callback) {
  if (!electron::Browser::Get()->is_ready()) {
    gin_helper::ErrorThrower(JavascriptEnvironment::GetIsolate())
        .ThrowError("globalShortcut cannot be used before the app is ready");
    return false;
  }
  std::vector<ui::Accelerator> registered;

  for (auto& accelerator : accelerators) {
    if (!Register(accelerator, callback)) {
      // 如果有任何快捷键失败，则取消注册所有快捷键。
      UnregisterSome(registered);
      return false;
    }

    registered.push_back(accelerator);
  }
  return true;
}

bool GlobalShortcut::Register(const ui::Accelerator& accelerator,
                              const base::RepeatingClosure& callback) {
  if (!electron::Browser::Get()->is_ready()) {
    gin_helper::ErrorThrower(JavascriptEnvironment::GetIsolate())
        .ThrowError("globalShortcut cannot be used before the app is ready");
    return false;
  }
#if defined(OS_MAC)
  if (RegisteringMediaKeyForUntrustedClient(accelerator))
    return false;
#endif

  if (!GlobalShortcutListener::GetInstance()->RegisterAccelerator(accelerator,
                                                                  this)) {
    return false;
  }

  accelerator_callback_map_[accelerator] = callback;
  return true;
}

void GlobalShortcut::Unregister(const ui::Accelerator& accelerator) {
  if (!electron::Browser::Get()->is_ready()) {
    gin_helper::ErrorThrower(JavascriptEnvironment::GetIsolate())
        .ThrowError("globalShortcut cannot be used before the app is ready");
    return;
  }
  if (accelerator_callback_map_.erase(accelerator) == 0)
    return;

  GlobalShortcutListener::GetInstance()->UnregisterAccelerator(accelerator,
                                                               this);
}

void GlobalShortcut::UnregisterSome(
    const std::vector<ui::Accelerator>& accelerators) {
  for (auto& accelerator : accelerators) {
    Unregister(accelerator);
  }
}

bool GlobalShortcut::IsRegistered(const ui::Accelerator& accelerator) {
  return base::Contains(accelerator_callback_map_, accelerator);
}

void GlobalShortcut::UnregisterAll() {
  if (!electron::Browser::Get()->is_ready()) {
    gin_helper::ErrorThrower(JavascriptEnvironment::GetIsolate())
        .ThrowError("globalShortcut cannot be used before the app is ready");
    return;
  }
  accelerator_callback_map_.clear();
  GlobalShortcutListener::GetInstance()->UnregisterAccelerators(this);
}

// 静电。
gin::Handle<GlobalShortcut> GlobalShortcut::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new GlobalShortcut(isolate));
}

// 静电。
gin::ObjectTemplateBuilder GlobalShortcut::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<GlobalShortcut>::GetObjectTemplateBuilder(isolate)
      .SetMethod("registerAll", &GlobalShortcut::RegisterAll)
      .SetMethod("register", &GlobalShortcut::Register)
      .SetMethod("isRegistered", &GlobalShortcut::IsRegistered)
      .SetMethod("unregister", &GlobalShortcut::Unregister)
      .SetMethod("unregisterAll", &GlobalShortcut::UnregisterAll);
}

const char* GlobalShortcut::GetTypeName() {
  return "GlobalShortcut";
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
  dict.Set("globalShortcut", electron::api::GlobalShortcut::Create(isolate));
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_global_shortcut, Initialize)
