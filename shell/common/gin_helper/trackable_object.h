// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_TRACKABLE_OBJECT_H_
#define SHELL_COMMON_GIN_HELPER_TRACKABLE_OBJECT_H_

#include <vector>

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/gin_helper/event_emitter.h"
#include "shell/common/key_weak_map.h"

namespace base {
class SupportsUserData;
}

namespace gin_helper {

// 用户应该改用TrackableObject。
class TrackableObjectBase : public CleanedUpAtExit {
 public:
  TrackableObjectBase();

  // 弱映射中的ID。
  int32_t weak_map_id() const { return weak_map_id_; }

  // 将TrackableObject包装到SupportsUserData类中。
  void AttachAsUserData(base::SupportsUserData* wrapped);

  // 从SupportsUserData获取弱映射id。
  static int32_t GetIDFromWrappedClass(base::SupportsUserData* wrapped);

 protected:
  ~TrackableObjectBase() override;

  // 返回可以销毁本机类的闭包。
  base::OnceClosure GetDestroyClosure();

  int32_t weak_map_id_ = 0;

 private:
  void Destroy();

  base::WeakPtrFactory<TrackableObjectBase> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(TrackableObjectBase);
};

// TrackableObject的所有实例都将保存在弱映射中，并且可以。
// 从它的身份证上看。
template <typename T>
class TrackableObject : public TrackableObjectBase, public EventEmitter<T> {
 public:
  // 将JS对象标记为已销毁。
  void MarkDestroyed() {
    v8::HandleScope scope(gin_helper::Wrappable<T>::isolate());
    v8::Local<v8::Object> wrapper = gin_helper::Wrappable<T>::GetWrapper();
    if (!wrapper.IsEmpty()) {
      wrapper->SetAlignedPointerInInternalField(0, nullptr);
      gin_helper::WrappableBase::wrapper_.ClearWeak();
    }
  }

  bool IsDestroyed() {
    v8::HandleScope scope(gin_helper::Wrappable<T>::isolate());
    v8::Local<v8::Object> wrapper = gin_helper::Wrappable<T>::GetWrapper();
    return wrapper->InternalFieldCount() == 0 ||
           wrapper->GetAlignedPointerFromInternalField(0) == nullptr;
  }

  // 在弱映射中从其ID中查找TrackableObject。
  static T* FromWeakMapID(v8::Isolate* isolate, int32_t id) {
    if (!weak_map_)
      return nullptr;

    v8::HandleScope scope(isolate);
    v8::MaybeLocal<v8::Object> object = weak_map_->Get(isolate, id);
    if (object.IsEmpty())
      return nullptr;

    T* self = nullptr;
    gin::ConvertFromV8(isolate, object.ToLocalChecked(), &self);
    return self;
  }

  // 从TrackableObject包装的类中查找它。
  static T* FromWrappedClass(v8::Isolate* isolate,
                             base::SupportsUserData* wrapped) {
    int32_t id = GetIDFromWrappedClass(wrapped);
    if (!id)
      return nullptr;
    return FromWeakMapID(isolate, id);
  }

  // 返回该类的弱映射中的所有对象。
  static std::vector<v8::Local<v8::Object>> GetAll(v8::Isolate* isolate) {
    if (weak_map_)
      return weak_map_->Values(isolate);
    else
      return std::vector<v8::Local<v8::Object>>();
  }

  // 从弱贴图中删除此实例。
  void RemoveFromWeakMap() {
    if (weak_map_ && weak_map_->Has(weak_map_id()))
      weak_map_->Remove(weak_map_id());
  }

 protected:
  TrackableObject() { weak_map_id_ = ++next_id_; }

  ~TrackableObject() override { RemoveFromWeakMap(); }

  void InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper) override {
    gin_helper::WrappableBase::InitWith(isolate, wrapper);
    if (!weak_map_) {
      weak_map_ = new electron::KeyWeakMap<int32_t>;
    }
    weak_map_->Set(isolate, weak_map_id_, wrapper);
  }

 private:
  static int32_t next_id_;
  static electron::KeyWeakMap<int32_t>* weak_map_;  // 故意泄露的。

  DISALLOW_COPY_AND_ASSIGN(TrackableObject);
};

template <typename T>
int32_t TrackableObject<T>::next_id_ = 0;

template <typename T>
electron::KeyWeakMap<int32_t>* TrackableObject<T>::weak_map_ = nullptr;

}  // 命名空间gin_helper。

#endif  // Shell_COMMON_GIN_HELPER_TRACKABLE_OBJECT_H_
