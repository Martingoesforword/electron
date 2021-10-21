// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_helper/destroyable.h"

#include "base/no_destructor.h"
#include "gin/converter.h"
#include "shell/common/gin_helper/wrappable_base.h"

namespace gin_helper {

namespace {

v8::Global<v8::FunctionTemplate>* GetDestroyFunc() {
  static base::NoDestructor<v8::Global<v8::FunctionTemplate>> destroy_func;
  return destroy_func.get();
}

v8::Global<v8::FunctionTemplate>* GetIsDestroyedFunc() {
  static base::NoDestructor<v8::Global<v8::FunctionTemplate>> is_destroyed_func;
  return is_destroyed_func.get();
}

void DestroyFunc(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> holder = info.Holder();
  if (Destroyable::IsDestroyed(holder))
    return;

  // TODO(Zcbenz)：gin_helper：：Wrappable将被删除。
  delete static_cast<gin_helper::WrappableBase*>(
      holder->GetAlignedPointerFromInternalField(0));
  holder->SetAlignedPointerInInternalField(0, nullptr);
}

void IsDestroyedFunc(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(gin::ConvertToV8(
      info.GetIsolate(), Destroyable::IsDestroyed(info.Holder())));
}

}  // 命名空间。

// 静电。
bool Destroyable::IsDestroyed(v8::Local<v8::Object> object) {
  // 如果对象没有内部指针或其。
  // 内部已经被摧毁了。
  return object->InternalFieldCount() == 0 ||
         object->GetAlignedPointerFromInternalField(0) == nullptr;
}

// 静电。
void Destroyable::MakeDestroyable(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  // 缓存“Destroy”和“isDestroed”的FunctionTemplate。
  if (GetDestroyFunc()->IsEmpty()) {
    auto templ = v8::FunctionTemplate::New(isolate, DestroyFunc);
    templ->RemovePrototype();
    GetDestroyFunc()->Reset(isolate, templ);
    templ = v8::FunctionTemplate::New(isolate, IsDestroyedFunc);
    templ->RemovePrototype();
    GetIsDestroyedFunc()->Reset(isolate, templ);
  }

  auto proto_templ = prototype->PrototypeTemplate();
  proto_templ->Set(
      gin::StringToSymbol(isolate, "destroy"),
      v8::Local<v8::FunctionTemplate>::New(isolate, *GetDestroyFunc()));
  proto_templ->Set(
      gin::StringToSymbol(isolate, "isDestroyed"),
      v8::Local<v8::FunctionTemplate>::New(isolate, *GetIsDestroyedFunc()));
}

}  // 命名空间gin_helper
