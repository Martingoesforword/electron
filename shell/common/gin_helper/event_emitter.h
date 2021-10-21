// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_
#define SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_

#include <utility>
#include <vector>

#include "content/public/browser/browser_thread.h"
#include "electron/shell/common/api/api.mojom.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/wrappable.h"

namespace content {
class RenderFrameHost;
}

namespace gin_helper {

namespace internal {

v8::Local<v8::Object> CreateCustomEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> sender = v8::Local<v8::Object>(),
    v8::Local<v8::Object> custom_event = v8::Local<v8::Object>());
v8::Local<v8::Object> CreateNativeEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> sender,
    content::RenderFrameHost* frame,
    electron::mojom::ElectronBrowser::MessageSyncCallback callback);

}  // 命名空间内部。

// 提供帮助器以在JavaScript中发出事件。
template <typename T>
class EventEmitter : public gin_helper::Wrappable<T> {
 public:
  using Base = gin_helper::Wrappable<T>;
  using ValueArray = std::vector<v8::Local<v8::Value>>;

  // 使方便的方法可见：
  // Https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members。
  v8::Isolate* isolate() const { return Base::isolate(); }
  v8::Local<v8::Object> GetWrapper() const { return Base::GetWrapper(); }
  v8::MaybeLocal<v8::Object> GetWrapper(v8::Isolate* isolate) const {
    return Base::GetWrapper(isolate);
  }

  // This.emit(名称，事件，参数...)；
  template <typename... Args>
  bool EmitCustomEvent(base::StringPiece name,
                       v8::Local<v8::Object> event,
                       Args&&... args) {
    return EmitWithEvent(
        name, internal::CreateCustomEvent(isolate(), GetWrapper(), event),
        std::forward<Args>(args)...);
  }

  // This.emit(name，new event()，args...)；
  template <typename... Args>
  bool Emit(base::StringPiece name, Args&&... args) {
    v8::Locker locker(isolate());
    v8::HandleScope handle_scope(isolate());
    v8::Local<v8::Object> wrapper = GetWrapper();
    if (wrapper.IsEmpty())
      return false;
    v8::Local<v8::Object> event =
        internal::CreateCustomEvent(isolate(), wrapper);
    return EmitWithEvent(name, event, std::forward<Args>(args)...);
  }

 protected:
  EventEmitter() {}

 private:
  // This.emit(名称，事件，参数...)；
  template <typename... Args>
  bool EmitWithEvent(base::StringPiece name,
                     v8::Local<v8::Object> event,
                     Args&&... args) {
    // EmitEvent可能会删除|此|文件，因此请保存所有内容。
    // 在调用EmitEvent之前，我们需要|this|。
    auto* isolate = this->isolate();
    auto context = isolate->GetCurrentContext();
    gin_helper::EmitEvent(isolate, GetWrapper(), name, event,
                          std::forward<Args>(args)...);
    v8::Local<v8::Value> defaultPrevented;
    if (event->Get(context, gin::StringToV8(isolate, "defaultPrevented"))
            .ToLocal(&defaultPrevented)) {
      return defaultPrevented->BooleanValue(isolate);
    }
    return false;
  }

  DISALLOW_COPY_AND_ASSIGN(EventEmitter);
};

}  // 命名空间gin_helper。

#endif  // Shell_COMMON_GIN_HELPER_EVENT_EMITER_H_
