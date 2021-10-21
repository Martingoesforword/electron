// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_PINNABLE_H_
#define SHELL_COMMON_GIN_HELPER_PINNABLE_H_

#include "v8/include/v8.h"

namespace gin_helper {

template <typename T>
class Pinnable {
 protected:
  // 在调用Unpin()之前防止对对象进行垃圾回收。
  void Pin(v8::Isolate* isolate) {
    v8::Locker locker(isolate);
    v8::HandleScope scope(isolate);
    v8::Local<v8::Value> wrapper;
    if (static_cast<T*>(this)->GetWrapper(isolate).ToLocal(&wrapper)) {
      pinned_.Reset(isolate, wrapper);
    }
  }

  // 允许对对象进行垃圾回收。
  void Unpin() { pinned_.Reset(); }

 private:
  v8::Global<v8::Value> pinned_;
};

}  // 命名空间gin_helper。

#endif  // SHELL_COMMON_GIN_HELPER_PINNABLE_H_
