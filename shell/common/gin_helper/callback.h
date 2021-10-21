// 版权所有(C)2019 GitHub，Inc.保留所有权利。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_CALLBACK_H_
#define SHELL_COMMON_GIN_HELPER_CALLBACK_H_

#include <utility>
#include <vector>

#include "base/bind.h"
#include "gin/dictionary.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/function_template.h"
#include "shell/common/gin_helper/locker.h"
#include "shell/common/gin_helper/microtasks_scope.h"
// 实现JS函数和base：：RepeatingCallback之间的安全转换。

namespace gin_helper {

template <typename T>
class RefCountedGlobal;

// 使用RAII管理V8功能。
class SafeV8Function {
 public:
  SafeV8Function(v8::Isolate* isolate, v8::Local<v8::Value> value);
  SafeV8Function(const SafeV8Function& other);
  ~SafeV8Function();

  bool IsAlive() const;
  v8::Local<v8::Function> NewHandle(v8::Isolate* isolate) const;

 private:
  scoped_refptr<RefCountedGlobal<v8::Function>> v8_function_;
};

// Helper使用C++参数调用V8函数。
template <typename Sig>
struct V8FunctionInvoker {};

template <typename... ArgTypes>
struct V8FunctionInvoker<v8::Local<v8::Value>(ArgTypes...)> {
  static v8::Local<v8::Value> Go(v8::Isolate* isolate,
                                 const SafeV8Function& function,
                                 ArgTypes... raw) {
    gin_helper::Locker locker(isolate);
    v8::EscapableHandleScope handle_scope(isolate);
    if (!function.IsAlive())
      return v8::Null(isolate);
    gin_helper::MicrotasksScope microtasks_scope(isolate, true);
    v8::Local<v8::Function> holder = function.NewHandle(isolate);
    v8::Local<v8::Context> context = holder->CreationContext();
    v8::Context::Scope context_scope(context);
    std::vector<v8::Local<v8::Value>> args{
        gin::ConvertToV8(isolate, std::forward<ArgTypes>(raw))...};
    v8::MaybeLocal<v8::Value> ret = holder->Call(
        context, holder, args.size(), args.empty() ? nullptr : &args.front());
    if (ret.IsEmpty())
      return v8::Undefined(isolate);
    else
      return handle_scope.Escape(ret.ToLocalChecked());
  }
};

template <typename... ArgTypes>
struct V8FunctionInvoker<void(ArgTypes...)> {
  static void Go(v8::Isolate* isolate,
                 const SafeV8Function& function,
                 ArgTypes... raw) {
    gin_helper::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    if (!function.IsAlive())
      return;
    gin_helper::MicrotasksScope microtasks_scope(isolate, true);
    v8::Local<v8::Function> holder = function.NewHandle(isolate);
    v8::Local<v8::Context> context = holder->CreationContext();
    v8::Context::Scope context_scope(context);
    std::vector<v8::Local<v8::Value>> args{
        gin::ConvertToV8(isolate, std::forward<ArgTypes>(raw))...};
    holder
        ->Call(context, holder, args.size(),
               args.empty() ? nullptr : &args.front())
        .IsEmpty();
  }
};

template <typename ReturnType, typename... ArgTypes>
struct V8FunctionInvoker<ReturnType(ArgTypes...)> {
  static ReturnType Go(v8::Isolate* isolate,
                       const SafeV8Function& function,
                       ArgTypes... raw) {
    gin_helper::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    ReturnType ret = ReturnType();
    if (!function.IsAlive())
      return ret;
    gin_helper::MicrotasksScope microtasks_scope(isolate, true);
    v8::Local<v8::Function> holder = function.NewHandle(isolate);
    v8::Local<v8::Context> context = holder->CreationContext();
    v8::Context::Scope context_scope(context);
    std::vector<v8::Local<v8::Value>> args{
        gin::ConvertToV8(isolate, std::forward<ArgTypes>(raw))...};
    v8::Local<v8::Value> result;
    auto maybe_result = holder->Call(context, holder, args.size(),
                                     args.empty() ? nullptr : &args.front());
    if (maybe_result.ToLocal(&result))
      gin::Converter<ReturnType>::FromV8(isolate, result, &ret);
    return ret;
  }
};

// Helper将C++函数传递给JavaScript。
using Translater = base::RepeatingCallback<void(gin::Arguments* args)>;
v8::Local<v8::Value> CreateFunctionFromTranslater(v8::Isolate* isolate,
                                                  const Translater& translater,
                                                  bool one_time);
v8::Local<v8::Value> BindFunctionWith(v8::Isolate* isolate,
                                      v8::Local<v8::Context> context,
                                      v8::Local<v8::Function> func,
                                      v8::Local<v8::Value> arg1,
                                      v8::Local<v8::Value> arg2);

// 使用参数调用回调。
template <typename Sig>
struct NativeFunctionInvoker {};

template <typename ReturnType, typename... ArgTypes>
struct NativeFunctionInvoker<ReturnType(ArgTypes...)> {
  static void Go(base::RepeatingCallback<ReturnType(ArgTypes...)> val,
                 gin::Arguments* args) {
    using Indices = typename IndicesGenerator<sizeof...(ArgTypes)>::type;
    Invoker<Indices, ArgTypes...> invoker(args, 0);
    if (invoker.IsOK())
      invoker.DispatchToCallback(val);
  }
};

// 将回调转换为V8，不受呼叫号的限制，这很容易实现。
// 导致内存泄漏，因此请谨慎使用。
template <typename Sig>
v8::Local<v8::Value> CallbackToV8Leaked(
    v8::Isolate* isolate,
    const base::RepeatingCallback<Sig>& val) {
  Translater translater =
      base::BindRepeating(&NativeFunctionInvoker<Sig>::Go, val);
  return CreateFunctionFromTranslater(isolate, translater, false);
}

}  // 命名空间gin_helper。

#endif  // SHELL_COMMON_GIN_HELPER_CALLBACK_H_
