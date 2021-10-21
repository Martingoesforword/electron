// 版权所有(C)2013 GitHub，Inc.。
// 版权所有(C)2012英特尔公司保留所有权利。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/api/object_life_monitor.h"

#include "base/bind.h"

namespace electron {

ObjectLifeMonitor::ObjectLifeMonitor(v8::Isolate* isolate,
                                     v8::Local<v8::Object> target)
    : target_(isolate, target) {
  target_.SetWeak(this, OnObjectGC, v8::WeakCallbackType::kParameter);
}

ObjectLifeMonitor::~ObjectLifeMonitor() {
  if (target_.IsEmpty())
    return;
  target_.ClearWeak();
  target_.Reset();
}

// 静电。
void ObjectLifeMonitor::OnObjectGC(
    const v8::WeakCallbackInfo<ObjectLifeMonitor>& data) {
  ObjectLifeMonitor* self = data.GetParameter();
  self->target_.Reset();
  self->RunDestructor();
  data.SetSecondPassCallback(Free);
}

// 静电。
void ObjectLifeMonitor::Free(
    const v8::WeakCallbackInfo<ObjectLifeMonitor>& data) {
  delete data.GetParameter();
}

}  // 命名空间电子
