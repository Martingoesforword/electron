// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_CONSTRUCTOR_H_
#define SHELL_COMMON_GIN_HELPER_CONSTRUCTOR_H_

#include "shell/common/gin_helper/function_template.h"
#include "shell/common/gin_helper/wrappable_base.h"

namespace gin_helper {

namespace internal {

// 这组模板调用base：：RepeatingCallback，方法是将。
// 参数转换为本机类型。它依赖函数_template.h来提供。
// 辅助对象模板。
inline WrappableBase* InvokeFactory(
    gin::Arguments* args,
    const base::RepeatingCallback<WrappableBase*()>& callback) {
  return callback.Run();
}

template <typename P1>
inline WrappableBase* InvokeFactory(
    gin::Arguments* args,
    const base::RepeatingCallback<WrappableBase*(P1)>& callback) {
  typename CallbackParamTraits<P1>::LocalType a1;
  if (!gin_helper::GetNextArgument(args, 0, true, &a1))
    return nullptr;
  return callback.Run(a1);
}

template <typename P1, typename P2>
inline WrappableBase* InvokeFactory(
    gin::Arguments* args,
    const base::RepeatingCallback<WrappableBase*(P1, P2)>& callback) {
  typename CallbackParamTraits<P1>::LocalType a1;
  typename CallbackParamTraits<P2>::LocalType a2;
  if (!gin_helper::GetNextArgument(args, 0, true, &a1) ||
      !gin_helper::GetNextArgument(args, 0, false, &a2))
    return nullptr;
  return callback.Run(a1, a2);
}

template <typename P1, typename P2, typename P3>
inline WrappableBase* InvokeFactory(
    gin::Arguments* args,
    const base::RepeatingCallback<WrappableBase*(P1, P2, P3)>& callback) {
  typename CallbackParamTraits<P1>::LocalType a1;
  typename CallbackParamTraits<P2>::LocalType a2;
  typename CallbackParamTraits<P3>::LocalType a3;
  if (!gin_helper::GetNextArgument(args, 0, true, &a1) ||
      !gin_helper::GetNextArgument(args, 0, false, &a2) ||
      !gin_helper::GetNextArgument(args, 0, false, &a3))
    return nullptr;
  return callback.Run(a1, a2, a3);
}

template <typename P1, typename P2, typename P3, typename P4>
inline WrappableBase* InvokeFactory(
    gin::Arguments* args,
    const base::RepeatingCallback<WrappableBase*(P1, P2, P3, P4)>& callback) {
  typename CallbackParamTraits<P1>::LocalType a1;
  typename CallbackParamTraits<P2>::LocalType a2;
  typename CallbackParamTraits<P3>::LocalType a3;
  typename CallbackParamTraits<P4>::LocalType a4;
  if (!gin_helper::GetNextArgument(args, 0, true, &a1) ||
      !gin_helper::GetNextArgument(args, 0, false, &a2) ||
      !gin_helper::GetNextArgument(args, 0, false, &a3) ||
      !gin_helper::GetNextArgument(args, 0, false, &a4))
    return nullptr;
  return callback.Run(a1, a2, a3, a4);
}

template <typename P1, typename P2, typename P3, typename P4, typename P5>
inline WrappableBase* InvokeFactory(
    gin::Arguments* args,
    const base::RepeatingCallback<WrappableBase*(P1, P2, P3, P4, P5)>&
        callback) {
  typename CallbackParamTraits<P1>::LocalType a1;
  typename CallbackParamTraits<P2>::LocalType a2;
  typename CallbackParamTraits<P3>::LocalType a3;
  typename CallbackParamTraits<P4>::LocalType a4;
  typename CallbackParamTraits<P5>::LocalType a5;
  if (!gin_helper::GetNextArgument(args, 0, true, &a1) ||
      !gin_helper::GetNextArgument(args, 0, false, &a2) ||
      !gin_helper::GetNextArgument(args, 0, false, &a3) ||
      !gin_helper::GetNextArgument(args, 0, false, &a4) ||
      !gin_helper::GetNextArgument(args, 0, false, &a5))
    return nullptr;
  return callback.Run(a1, a2, a3, a4, a5);
}

template <typename P1,
          typename P2,
          typename P3,
          typename P4,
          typename P5,
          typename P6>
inline WrappableBase* InvokeFactory(
    gin::Arguments* args,
    const base::RepeatingCallback<WrappableBase*(P1, P2, P3, P4, P5, P6)>&
        callback) {
  typename CallbackParamTraits<P1>::LocalType a1;
  typename CallbackParamTraits<P2>::LocalType a2;
  typename CallbackParamTraits<P3>::LocalType a3;
  typename CallbackParamTraits<P4>::LocalType a4;
  typename CallbackParamTraits<P5>::LocalType a5;
  typename CallbackParamTraits<P6>::LocalType a6;
  if (!gin_helper::GetNextArgument(args, 0, true, &a1) ||
      !gin_helper::GetNextArgument(args, 0, false, &a2) ||
      !gin_helper::GetNextArgument(args, 0, false, &a3) ||
      !gin_helper::GetNextArgument(args, 0, false, &a4) ||
      !gin_helper::GetNextArgument(args, 0, false, &a5) ||
      !gin_helper::GetNextArgument(args, 0, false, &a6))
    return nullptr;
  return callback.Run(a1, a2, a3, a4, a5, a6);
}

template <typename Sig>
void InvokeNew(const base::RepeatingCallback<Sig>& factory,
               v8::Isolate* isolate,
               gin_helper::Arguments* args) {
  if (!args->IsConstructCall()) {
    args->ThrowError("Requires constructor call");
    return;
  }

  WrappableBase* object;
  {
    // 如果构造函数抛出异常，请不要继续。
    v8::TryCatch try_catch(isolate);
    object = internal::InvokeFactory(args, factory);
    if (try_catch.HasCaught()) {
      try_catch.ReThrow();
      return;
    }
  }

  if (!object)
    args->ThrowError();

  return;
}

}  // 命名空间内部。

// 创建一个可以在JavaScript中“新建”的FunctionTemplate。
// 用户有责任确保为一种类型调用此函数。
// 在程序的整个生命周期中只有一次，否则我们就会有内存。
// 漏水了。
template <typename T, typename Sig>
v8::Local<v8::Function> CreateConstructor(
    v8::Isolate* isolate,
    const base::RepeatingCallback<Sig>& func) {
#ifndef NDEBUG
  static bool called = false;
  CHECK(!called) << "CreateConstructor can only be called for one type once";
  called = true;
#endif
  v8::Local<v8::FunctionTemplate> templ = gin_helper::CreateFunctionTemplate(
      isolate, base::BindRepeating(&internal::InvokeNew<Sig>, func));
  templ->InstanceTemplate()->SetInternalFieldCount(1);
  T::BuildPrototype(isolate, templ);
  return templ->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
}

}  // 命名空间gin_helper。

#endif  // Shell_common_gin_helper_structor_H_
