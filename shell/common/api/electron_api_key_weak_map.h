// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_API_ELECTRON_API_KEY_WEAK_MAP_H_
#define SHELL_COMMON_API_ELECTRON_API_KEY_WEAK_MAP_H_

#include "gin/handle.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/wrappable.h"
#include "shell/common/key_weak_map.h"

namespace electron {

namespace api {

template <typename K>
class KeyWeakMap : public gin_helper::Wrappable<KeyWeakMap<K>> {
 public:
  static gin::Handle<KeyWeakMap<K>> Create(v8::Isolate* isolate) {
    return gin::CreateHandle(isolate, new KeyWeakMap<K>(isolate));
  }

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype) {
    prototype->SetClassName(gin::StringToV8(isolate, "KeyWeakMap"));
    gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
        .SetMethod("set", &KeyWeakMap<K>::Set)
        .SetMethod("get", &KeyWeakMap<K>::Get)
        .SetMethod("has", &KeyWeakMap<K>::Has)
        .SetMethod("remove", &KeyWeakMap<K>::Remove);
  }

 protected:
  explicit KeyWeakMap(v8::Isolate* isolate) {
    gin_helper::Wrappable<KeyWeakMap<K>>::Init(isolate);
  }
  ~KeyWeakMap() override {}

 private:
  // KeyWeakMap接口。
  void Set(v8::Isolate* isolate, const K& key, v8::Local<v8::Object> object) {
    key_weak_map_.Set(isolate, key, object);
  }

  v8::Local<v8::Object> Get(v8::Isolate* isolate, const K& key) {
    return key_weak_map_.Get(isolate, key).ToLocalChecked();
  }

  bool Has(const K& key) { return key_weak_map_.Has(key); }

  void Remove(const K& key) { key_weak_map_.Remove(key); }

  electron::KeyWeakMap<K> key_weak_map_;

  DISALLOW_COPY_AND_ASSIGN(KeyWeakMap);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_COMMON_API_ELEMENT_API_KEY_弱_MAP_H_
