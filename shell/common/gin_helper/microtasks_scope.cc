// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_helper/microtasks_scope.h"

#include "shell/common/gin_helper/locker.h"

namespace gin_helper {

MicrotasksScope::MicrotasksScope(v8::Isolate* isolate,
                                 bool ignore_browser_checkpoint,
                                 v8::MicrotasksScope::Type scope_type) {
  if (Locker::IsBrowserProcess()) {
    if (!ignore_browser_checkpoint)
      v8::MicrotasksScope::PerformCheckpoint(isolate);
  } else {
    v8_microtasks_scope_ =
        std::make_unique<v8::MicrotasksScope>(isolate, scope_type);
  }
}

MicrotasksScope::~MicrotasksScope() = default;

}  // 命名空间gin_helper
