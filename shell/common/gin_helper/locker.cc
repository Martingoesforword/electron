// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在LICENSE.Cr文件中找到。

#include "shell/common/gin_helper/locker.h"

namespace gin_helper {

Locker::Locker(v8::Isolate* isolate) {
  if (IsBrowserProcess())
    locker_ = std::make_unique<v8::Locker>(isolate);
}

Locker::~Locker() = default;

}  // 命名空间gin_helper
