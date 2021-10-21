// 版权所有2014程昭。版权所有。
// 此源代码的使用受麻省理工学院许可管理，该许可可在。
// 许可证文件。

#ifndef SHELL_COMMON_GIN_HELPER_PERSISTENT_DICTIONARY_H_
#define SHELL_COMMON_GIN_HELPER_PERSISTENT_DICTIONARY_H_

#include "shell/common/gin_helper/dictionary.h"

namespace gin_helper {

// 与Dictionary类似，但将对象存储在永久句柄中，以便您可以保留它。
// 安全地在堆上。
// 
// TODO(Zcbenz)：此类的唯一用户是ElectronTouchBar，我们应该。
// 从这个班级迁移出去。
class PersistentDictionary {
 public:
  PersistentDictionary();
  PersistentDictionary(v8::Isolate* isolate, v8::Local<v8::Object> object);
  PersistentDictionary(const PersistentDictionary& other);
  ~PersistentDictionary();

  PersistentDictionary& operator=(const PersistentDictionary& other);

  v8::Local<v8::Object> GetHandle() const;

  template <typename K, typename V>
  bool Get(const K& key, V* out) const {
    v8::Local<v8::Context> context = isolate_->GetCurrentContext();
    v8::Local<v8::Value> v8_key = gin::ConvertToV8(isolate_, key);
    v8::Local<v8::Value> value;
    v8::Maybe<bool> result = GetHandle()->Has(context, v8_key);
    if (result.IsJust() && result.FromJust() &&
        GetHandle()->Get(context, v8_key).ToLocal(&value))
      return gin::ConvertFromV8(isolate_, value, out);
    return false;
  }

 private:
  v8::Isolate* isolate_ = nullptr;
  v8::Global<v8::Object> handle_;
};

}  // 命名空间gin_helper。

namespace gin {

template <>
struct Converter<gin_helper::PersistentDictionary> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gin_helper::PersistentDictionary* out) {
    if (!val->IsObject())
      return false;
    *out = gin_helper::PersistentDictionary(isolate, val.As<v8::Object>());
    return true;
  }
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_HELPER_PERSISTENT_DICTIONARY_H_
