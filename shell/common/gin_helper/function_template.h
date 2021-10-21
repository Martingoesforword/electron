// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在LICENSE.Cr文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_H_
#define SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_H_

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "gin/arguments.h"
#include "shell/common/gin_helper/arguments.h"
#include "shell/common/gin_helper/destroyable.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// 此文件是从gin/function_template.h派生的，有两个不同之处：
// 1.支持其他类型的参数。
// 2.支持销毁对象告警。
// 
// TODO(Zcbenz)：在删除NATIVE_Mate之后，我们应该设法删除此文件。

namespace gin_helper {

enum CreateFunctionTemplateFlags {
  HolderIsFirstArgument = 1 << 0,
};

template <typename T>
struct CallbackParamTraits {
  typedef T LocalType;
};
template <typename T>
struct CallbackParamTraits<const T&> {
  typedef T LocalType;
};
template <typename T>
struct CallbackParamTraits<const T*> {
  typedef T* LocalType;
};

// CallbackHolder和CallbackHolderBase用于传递。
// Base：：RepeatingCallback From。
// CreateFunctionTemplate至V8(通过V8：：FunctionTemplate)。
// DispatchToCallback，在其中调用它。

// 使用这个简单的基类是为了让我们可以共享单个对象模板。
// 在每个CallbackHolder实例中。
class CallbackHolderBase {
 public:
  v8::Local<v8::External> GetHandle(v8::Isolate* isolate);

 protected:
  explicit CallbackHolderBase(v8::Isolate* isolate);
  virtual ~CallbackHolderBase();

 private:
  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<CallbackHolderBase>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<CallbackHolderBase>& data);

  v8::Global<v8::External> v8_ref_;

  DISALLOW_COPY_AND_ASSIGN(CallbackHolderBase);
};

template <typename Sig>
class CallbackHolder : public CallbackHolderBase {
 public:
  CallbackHolder(v8::Isolate* isolate,
                 const base::RepeatingCallback<Sig>& callback,
                 int flags)
      : CallbackHolderBase(isolate), callback(callback), flags(flags) {}
  base::RepeatingCallback<Sig> callback;
  int flags = 0;

 private:
  virtual ~CallbackHolder() = default;

  DISALLOW_COPY_AND_ASSIGN(CallbackHolder);
};

template <typename T>
bool GetNextArgument(gin::Arguments* args,
                     int create_flags,
                     bool is_first,
                     T* result) {
  if (is_first && (create_flags & HolderIsFirstArgument) != 0) {
    return args->GetHolder(result);
  } else {
    return args->GetNext(result);
  }
}

// 支持ABSL：：OPTIONAL作为输出，为空且不抛出错误。
// 当转换为T失败时。
template <typename T>
bool GetNextArgument(gin::Arguments* args,
                     int create_flags,
                     bool is_first,
                     absl::optional<T>* result) {
  T converted;
  // 使用gin：：Arguments：：GetNext，它始终前进|Next|Counter。
  if (args->GetNext(&converted))
    result->emplace(std::move(converted));
  return true;
}

// 对于高级用例，我们允许调用方请求未解析的参数。
// 对象，并直接在其中四处窥探。
inline bool GetNextArgument(gin::Arguments* args,
                            int create_flags,
                            bool is_first,
                            gin::Arguments** result) {
  *result = args;
  return true;
}

// 对于客户来说，只需要隔离是很常见的，所以我们让它变得简单。
inline bool GetNextArgument(gin::Arguments* args,
                            int create_flags,
                            bool is_first,
                            v8::Isolate** result) {
  *result = args->isolate();
  return true;
}

// 允许客户端传递util：：Error以在以下情况下引发错误。
// 不需要完整的杜松子酒：：参数。
inline bool GetNextArgument(gin::Arguments* args,
                            int create_flags,
                            bool is_first,
                            ErrorThrower* result) {
  *result = ErrorThrower(args->isolate());
  return true;
}

// 支持gin_helper：：参数。
inline bool GetNextArgument(gin::Arguments* args,
                            int create_flags,
                            bool is_first,
                            gin_helper::Arguments** result) {
  *result = static_cast<gin_helper::Arguments*>(args);
  return true;
}

// 用于生成和存储整数索引的参数包的类。
// (基于广为人知的“指数把戏”，见：http://goo.gl/bKKojn)：
template <size_t... indices>
struct IndicesHolder {};

template <size_t requested_index, size_t... indices>
struct IndicesGenerator {
  using type = typename IndicesGenerator<requested_index - 1,
                                         requested_index - 1,
                                         indices...>::type;
};
template <size_t... indices>
struct IndicesGenerator<0, indices...> {
  using type = IndicesHolder<indices...>;
};

// 用于提取和存储回调的单个参数的类模板。
// 在位置|索引|。
template <size_t index, typename ArgType>
struct ArgumentHolder {
  using ArgLocalType = typename CallbackParamTraits<ArgType>::LocalType;

  ArgLocalType value;
  bool ok = false;

  ArgumentHolder(gin::Arguments* args, int create_flags) {
    v8::Local<v8::Object> holder;
    if (index == 0 && (create_flags & HolderIsFirstArgument) &&
        args->GetHolder(&holder) &&
        gin_helper::Destroyable::IsDestroyed(holder)) {
      args->ThrowTypeError("Object has been destroyed");
      return;
    }
    ok = GetNextArgument(args, create_flags, index == 0, &value);
    if (!ok) {
      // 理想情况下，我们应该在错误中包含预期的c++类型。
      // 我们可以通过typeid(ArgType).name()访问的消息。
      // 但是，我们使用no-RTTI进行编译，这会禁用typeid。
      args->ThrowError();
    }
  }
};

// 用于将参数从JavaScript转换为C++并运行的类模板。
// 和他们一起回电。
template <typename IndicesType, typename... ArgTypes>
class Invoker {};

template <size_t... indices, typename... ArgTypes>
class Invoker<IndicesHolder<indices...>, ArgTypes...>
    : public ArgumentHolder<indices, ArgTypes>... {
 public:
  // 对于每个参数，调用器&lt;&gt;都继承自ArgumentHolder&lt;&gt;。
  // C++对类的初始化顺序一直很严格，
  // 因此可以保证ArgumentHolders将被初始化(因此，
  // 以正确的顺序从参数中提取参数)。
  Invoker(gin::Arguments* args, int create_flags)
      : ArgumentHolder<indices, ArgTypes>(args, create_flags)..., args_(args) {
    // GCC认为CREATE_FLAGS没有使用，即使。
    // 上面的扩张显然利用了它。Per Jyasskin@，选角。
    // VALID是通常接受的说服编译器的方式。
    // 您实际上正在使用一个参数/变量。
    (void)create_flags;
  }

  bool IsOK() { return And(ArgumentHolder<indices, ArgTypes>::ok...); }

  template <typename ReturnType>
  void DispatchToCallback(
      base::RepeatingCallback<ReturnType(ArgTypes...)> callback) {
    gin_helper::MicrotasksScope microtasks_scope(args_->isolate(), true);
    args_->Return(
        callback.Run(std::move(ArgumentHolder<indices, ArgTypes>::value)...));
  }

  // 在C++中，可以声明函数foo(Void)，但不能传递void。
  // Foo的表达式。因此，我们必须专门化回调的情况。
  // 具有void返回类型的。
  void DispatchToCallback(base::RepeatingCallback<void(ArgTypes...)> callback) {
    gin_helper::MicrotasksScope microtasks_scope(args_->isolate(), true);
    callback.Run(std::move(ArgumentHolder<indices, ArgTypes>::value)...);
  }

 private:
  static bool And() { return true; }
  template <typename... T>
  static bool And(bool arg1, T... args) {
    return arg1 && And(args...);
  }

  gin::Arguments* args_;
};

// DispatchToCallback将所有JavaScript参数转换为C++类型。
// 调用base：：RepeatingCallback。
template <typename Sig>
struct Dispatcher {};

template <typename ReturnType, typename... ArgTypes>
struct Dispatcher<ReturnType(ArgTypes...)> {
  static void DispatchToCallback(
      const v8::FunctionCallbackInfo<v8::Value>& info) {
    gin::Arguments args(info);
    v8::Local<v8::External> v8_holder;
    args.GetData(&v8_holder);
    CallbackHolderBase* holder_base =
        reinterpret_cast<CallbackHolderBase*>(v8_holder->Value());

    typedef CallbackHolder<ReturnType(ArgTypes...)> HolderT;
    HolderT* holder = static_cast<HolderT*>(holder_base);

    using Indices = typename IndicesGenerator<sizeof...(ArgTypes)>::type;
    Invoker<Indices, ArgTypes...> invoker(&args, holder->flags);
    if (invoker.IsOK())
      invoker.DispatchToCallback(holder->callback);
  }
};

// CreateFunctionTemplate创建一个V8：：FunctionTemplate，该模板将创建。
// 执行所提供的C++函数的JavaScript函数或。
// Base：：RepeatingCallback。
// JavaScript参数按原样通过gin：：Converter自动转换。
// C++函数的返回值(如果有)。
// 
// 注意：V8缓存FunctionTemplate的时间为其自己的网页的整个生命周期。
// 内部原因，因此缓存模板通常是个好主意。
// 由此函数返回。否则，来自JS的重复方法调用。
// 会造成大量内存泄漏。请参阅http://crbug.com/463487.。
template <typename Sig>
v8::Local<v8::FunctionTemplate> CreateFunctionTemplate(
    v8::Isolate* isolate,
    const base::RepeatingCallback<Sig> callback,
    int callback_flags = 0) {
  typedef CallbackHolder<Sig> HolderT;
  HolderT* holder = new HolderT(isolate, callback, callback_flags);

  return v8::FunctionTemplate::New(isolate,
                                   &Dispatcher<Sig>::DispatchToCallback,
                                   gin::ConvertToV8<v8::Local<v8::External>>(
                                       isolate, holder->GetHandle(isolate)));
}

// 基本模板-仅用于非成员函数指针。其他类型。
// 要么转到以下专业化认证之一，要么转到此处编译失败。
// 因为base：：bind()。
template <typename T, typename Enable = void>
struct CallbackTraits {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(v8::Isolate* isolate,
                                                        T callback) {
    return gin_helper::CreateFunctionTemplate(isolate,
                                              base::BindRepeating(callback));
  }
};

// Base：：RepeatingCallback的专门化。
template <typename T>
struct CallbackTraits<base::RepeatingCallback<T>> {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(
      v8::Isolate* isolate,
      const base::RepeatingCallback<T>& callback) {
    return gin_helper::CreateFunctionTemplate(isolate, callback);
  }
};

// 成员函数指针的专门化。我们需要处理这个案子。
// 特别是因为用于回调MFP的第一个参数通常应该。
// 来自调用函数的JavaScript“this”对象，而不是。
// 从第一个法线参数开始。
template <typename T>
struct CallbackTraits<
    T,
    typename std::enable_if<std::is_member_function_pointer<T>::value>::type> {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(v8::Isolate* isolate,
                                                        T callback) {
    int flags = HolderIsFirstArgument;
    return gin_helper::CreateFunctionTemplate(
        isolate, base::BindRepeating(callback), flags);
  }
};

}  // 命名空间gin_helper。

#endif  // Shell_COMMON_GIN_HELPER_Function_Template_H_
