// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_DICTIONARY_H_
#define SHELL_COMMON_GIN_HELPER_DICTIONARY_H_

#include <type_traits>
#include <utility>

#include "gin/dictionary.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/accessor.h"
#include "shell/common/gin_helper/function_template.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace gin_helper {

// 向gin：：DICTIONARY添加更多扩展方法。
// 
// 请注意，作为gin：：DICTIONARY的析构函数，它不是虚拟的，我们希望。
// 在两种类型之间转换，我们不能添加任何成员。
class Dictionary : public gin::Dictionary {
 public:
  Dictionary() : gin::Dictionary(nullptr) {}
  Dictionary(v8::Isolate* isolate, v8::Local<v8::Object> object)
      : gin::Dictionary(isolate, object) {}

  // 允许从gin：：DICTIONARY隐式转换，因为它绝对。
  // 在这种情况下是安全的。
  Dictionary(const gin::Dictionary& dict)  // NOLINT(运行时/显式)。
      : gin::Dictionary(dict) {}

  // 与gin：：DICTIONARY中的GET方法不同：
  // 1.这是一个常量方法；
  // 2.读取前检查密钥是否存在；
  // 3.接受任意类型的密钥。
  template <typename K, typename V>
  bool Get(const K& key, V* out) const {
    // 在获取之前检查是否存在，否则此方法将始终。
    // 当T==v8：：local&lt;v8：：value&gt;时返回TRUE。
    v8::Local<v8::Context> context = isolate()->GetCurrentContext();
    v8::Local<v8::Value> v8_key = gin::ConvertToV8(isolate(), key);
    v8::Local<v8::Value> value;
    v8::Maybe<bool> result = GetHandle()->Has(context, v8_key);
    if (result.IsJust() && result.FromJust() &&
        GetHandle()->Get(context, v8_key).ToLocal(&value))
      return gin::ConvertFromV8(isolate(), value, out);
    return false;
  }

  // 与gin：：DICTIONARY中的set方法不同：
  // 1.接受任意类型的密钥。
  template <typename K, typename V>
  bool Set(const K& key, const V& val) {
    v8::Local<v8::Value> v8_value;
    if (!gin::TryConvertToV8(isolate(), val, &v8_value))
      return false;
    v8::Maybe<bool> result =
        GetHandle()->Set(isolate()->GetCurrentContext(),
                         gin::ConvertToV8(isolate(), key), v8_value);
    return !result.IsNothing() && result.FromJust();
  }

  // 与普通GET类似，但PUT结果为ABSL：：Optional。
  template <typename T>
  bool GetOptional(base::StringPiece key, absl::optional<T>* out) const {
    T ret;
    if (Get(key, &ret)) {
      out->emplace(std::move(ret));
      return true;
    } else {
      return false;
    }
  }

  template <typename T>
  bool GetHidden(base::StringPiece key, T* out) const {
    v8::Local<v8::Context> context = isolate()->GetCurrentContext();
    v8::Local<v8::Private> privateKey =
        v8::Private::ForApi(isolate(), gin::StringToV8(isolate(), key));
    v8::Local<v8::Value> value;
    v8::Maybe<bool> result = GetHandle()->HasPrivate(context, privateKey);
    if (result.IsJust() && result.FromJust() &&
        GetHandle()->GetPrivate(context, privateKey).ToLocal(&value))
      return gin::ConvertFromV8(isolate(), value, out);
    return false;
  }

  template <typename T>
  bool SetHidden(base::StringPiece key, T val) {
    v8::Local<v8::Value> v8_value;
    if (!gin::TryConvertToV8(isolate(), val, &v8_value))
      return false;
    v8::Local<v8::Context> context = isolate()->GetCurrentContext();
    v8::Local<v8::Private> privateKey =
        v8::Private::ForApi(isolate(), gin::StringToV8(isolate(), key));
    v8::Maybe<bool> result =
        GetHandle()->SetPrivate(context, privateKey, v8_value);
    return !result.IsNothing() && result.FromJust();
  }

  template <typename T>
  bool SetMethod(base::StringPiece key, const T& callback) {
    auto context = isolate()->GetCurrentContext();
    auto templ = CallbackTraits<T>::CreateTemplate(isolate(), callback);
    return GetHandle()
        ->Set(context, gin::StringToV8(isolate(), key),
              templ->GetFunction(context).ToLocalChecked())
        .ToChecked();
  }

  template <typename K, typename V>
  bool SetGetter(const K& key,
                 const V& val,
                 v8::PropertyAttribute attribute = v8::None) {
    AccessorValue<V> acc_value;
    acc_value.Value = val;

    v8::Local<v8::Value> v8_value_accessor;
    if (!gin::TryConvertToV8(isolate(), acc_value, &v8_value_accessor))
      return false;

    auto context = isolate()->GetCurrentContext();

    return GetHandle()
        ->SetAccessor(
            context, gin::StringToV8(isolate(), key),
            [](v8::Local<v8::Name> property_name,
               const v8::PropertyCallbackInfo<v8::Value>& info) {
              AccessorValue<V> acc_value;
              if (!gin::ConvertFromV8(info.GetIsolate(), info.Data(),
                                      &acc_value))
                return;

              V val = acc_value.Value;
              v8::Local<v8::Value> v8_value;
              if (gin::TryConvertToV8(info.GetIsolate(), val, &v8_value))
                info.GetReturnValue().Set(v8_value);
            },
            nullptr, v8_value_accessor, v8::DEFAULT, attribute)
        .ToChecked();
  }

  template <typename T>
  bool SetReadOnly(base::StringPiece key, const T& val) {
    v8::Local<v8::Value> v8_value;
    if (!gin::TryConvertToV8(isolate(), val, &v8_value))
      return false;
    v8::Maybe<bool> result = GetHandle()->DefineOwnProperty(
        isolate()->GetCurrentContext(), gin::StringToV8(isolate(), key),
        v8_value, v8::ReadOnly);
    return !result.IsNothing() && result.FromJust();
  }

  // 注意：如果我们计划添加更多的set方法，请考虑添加一个选项。
  // 抄袭代码。
  template <typename T>
  bool SetReadOnlyNonConfigurable(base::StringPiece key, T val) {
    v8::Local<v8::Value> v8_value;
    if (!gin::TryConvertToV8(isolate(), val, &v8_value))
      return false;
    v8::Maybe<bool> result = GetHandle()->DefineOwnProperty(
        isolate()->GetCurrentContext(), gin::StringToV8(isolate(), key),
        v8_value,
        static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));
    return !result.IsNothing() && result.FromJust();
  }

  bool Has(base::StringPiece key) const {
    v8::Maybe<bool> result = GetHandle()->Has(isolate()->GetCurrentContext(),
                                              gin::StringToV8(isolate(), key));
    return !result.IsNothing() && result.FromJust();
  }

  bool Delete(base::StringPiece key) {
    v8::Maybe<bool> result = GetHandle()->Delete(
        isolate()->GetCurrentContext(), gin::StringToV8(isolate(), key));
    return !result.IsNothing() && result.FromJust();
  }

  bool IsEmpty() const { return isolate() == nullptr || GetHandle().IsEmpty(); }

  v8::Local<v8::Object> GetHandle() const {
    return gin::ConvertToV8(isolate(),
                            *static_cast<const gin::Dictionary*>(this))
        .As<v8::Object>();
  }

 private:
  // 请勿添加任何数据成员。
};

}  // 命名空间gin_helper。

namespace gin {

template <>
struct Converter<gin_helper::Dictionary> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   gin_helper::Dictionary val) {
    return val.GetHandle();
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gin_helper::Dictionary* out) {
    gin::Dictionary gdict(isolate);
    if (!ConvertFromV8(isolate, val, &gdict))
      return false;
    *out = gin_helper::Dictionary(gdict);
    return true;
  }
};

}  // 命名空间杜松子酒。

#endif  // Shell_COMMON_GIN_HELPER_DICTIONARY_H_
