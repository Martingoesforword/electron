// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_API_OBJECT_LIFE_MONITOR_H_
#define SHELL_COMMON_API_OBJECT_LIFE_MONITOR_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "v8/include/v8.h"

namespace electron {

class ObjectLifeMonitor {
 protected:
  ObjectLifeMonitor(v8::Isolate* isolate, v8::Local<v8::Object> target);
  virtual ~ObjectLifeMonitor();

  virtual void RunDestructor() = 0;

 private:
  static void OnObjectGC(const v8::WeakCallbackInfo<ObjectLifeMonitor>& data);
  static void Free(const v8::WeakCallbackInfo<ObjectLifeMonitor>& data);

  v8::Global<v8::Object> target_;

  base::WeakPtrFactory<ObjectLifeMonitor> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ObjectLifeMonitor);
};

}  // 命名空间电子。

#endif  // Shell_COMMON_API_OBJECT_LIFE_MONITOR_H_
