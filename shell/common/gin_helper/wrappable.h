// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在LICENSE.Cr文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_WRAPPABLE_H_
#define SHELL_COMMON_GIN_HELPER_WRAPPABLE_H_

#include "base/bind.h"
#include "gin/per_isolate_data.h"
#include "shell/common/gin_helper/constructor.h"

namespace gin_helper {

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Local<v8::Value> val);

}  // 命名空间内部。

template <typename T>
class Wrappable : public WrappableBase {
 public:
  Wrappable() = default;

  template <typename Sig>
  static void SetConstructor(v8::Isolate* isolate,
                             const base::RepeatingCallback<Sig>& constructor) {
    v8::Local<v8::FunctionTemplate> templ = gin_helper::CreateFunctionTemplate(
        isolate, base::BindRepeating(&internal::InvokeNew<Sig>, constructor));
    templ->InstanceTemplate()->SetInternalFieldCount(1);
    T::BuildPrototype(isolate, templ);
    gin::PerIsolateData::From(isolate)->SetFunctionTemplate(&kWrapperInfo,
                                                            templ);
  }

  static v8::Local<v8::FunctionTemplate> GetConstructor(v8::Isolate* isolate) {
    // 填充对象模板。
    auto* data = gin::PerIsolateData::From(isolate);
    auto templ = data->GetFunctionTemplate(&kWrapperInfo);
    if (templ.IsEmpty()) {
      templ = v8::FunctionTemplate::New(isolate);
      templ->InstanceTemplate()->SetInternalFieldCount(1);
      T::BuildPrototype(isolate, templ);
      data->SetFunctionTemplate(&kWrapperInfo, templ);
    }
    return templ;
  }

 protected:
  // 使用T：：BuildPrototype初始化类。
  void Init(v8::Isolate* isolate) {
    v8::Local<v8::FunctionTemplate> templ = GetConstructor(isolate);

    // |Wrapper|在某些极端情况下可能为空，例如。
    // Object.Prototype.structor被覆盖。
    v8::Local<v8::Object> wrapper;
    if (!templ->InstanceTemplate()
             ->NewInstance(isolate->GetCurrentContext())
             .ToLocal(&wrapper)) {
      // 当前的可包装对象将不再由V8管理。删除。
      // 这就是现在。
      delete this;
      return;
    }
    InitWith(isolate, wrapper);
  }

 private:
  static gin::WrapperInfo kWrapperInfo;

  DISALLOW_COPY_AND_ASSIGN(Wrappable);
};

// 静电。
template <typename T>
gin::WrapperInfo Wrappable<T>::kWrapperInfo = {gin::kEmbedderNativeGin};

}  // 命名空间gin_helper。

namespace gin {

template <typename T>
struct Converter<
    T*,
    typename std::enable_if<
        std::is_convertible<T*, gin_helper::WrappableBase*>::value>::type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, T* val) {
    if (val)
      return val->GetWrapper();
    else
      return v8::Null(isolate);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val, T** out) {
    *out = static_cast<T*>(static_cast<gin_helper::WrappableBase*>(
        gin_helper::internal::FromV8Impl(isolate, val)));
    return *out != nullptr;
  }
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_HELPER_WRAPPABLE_H_
