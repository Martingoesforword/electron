// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_KEY_WEAK_MAP_H_
#define SHELL_COMMON_KEY_WEAK_MAP_H_

#include <unordered_map>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "v8/include/v8.h"

namespace electron {

// 与ES6的WeakMap类似，但键是整数，值是弱指针。
template <typename K>
class KeyWeakMap {
 public:
  // 记录SetWeak使用的密钥和自身。
  struct KeyObject {
    K key;
    KeyWeakMap* self;
  };

  KeyWeakMap() {}
  virtual ~KeyWeakMap() {
    for (auto& p : map_)
      p.second.second.ClearWeak();
  }

  // 使用给定的|key|将对象设置为WeakMap。
  void Set(v8::Isolate* isolate, const K& key, v8::Local<v8::Object> object) {
    KeyObject key_object = {key, this};
    auto& p = map_[key] =
        std::make_pair(key_object, v8::Global<v8::Object>(isolate, object));
    p.second.SetWeak(&(p.first), OnObjectGC, v8::WeakCallbackType::kParameter);
  }

  // 通过WeakMap的|key|从WeakMap获取对象。
  v8::MaybeLocal<v8::Object> Get(v8::Isolate* isolate, const K& key) {
    auto iter = map_.find(key);
    if (iter == map_.end())
      return v8::MaybeLocal<v8::Object>();
    else
      return v8::Local<v8::Object>::New(isolate, iter->second.second);
  }

  // 此WeakMap中是否存在带有|key|的对象。
  bool Has(const K& key) const { return map_.find(key) != map_.end(); }

  // 返回所有对象。
  std::vector<v8::Local<v8::Object>> Values(v8::Isolate* isolate) const {
    std::vector<v8::Local<v8::Object>> keys;
    keys.reserve(map_.size());
    for (const auto& it : map_)
      keys.emplace_back(v8::Local<v8::Object>::New(isolate, it.second.second));
    return keys;
  }

  // 在WeakMap中使用|Key|删除对象。
  void Remove(const K& key) {
    auto iter = map_.find(key);
    if (iter == map_.end())
      return;

    iter->second.second.ClearWeak();
    map_.erase(iter);
  }

 private:
  static void OnObjectGC(
      const v8::WeakCallbackInfo<typename KeyWeakMap<K>::KeyObject>& data) {
    KeyWeakMap<K>::KeyObject* key_object = data.GetParameter();
    key_object->self->Remove(key_object->key);
  }

  // 存储对象的地图。
  std::unordered_map<K, std::pair<KeyObject, v8::Global<v8::Object>>> map_;

  DISALLOW_COPY_AND_ASSIGN(KeyWeakMap);
};

}  // 命名空间电子。

#endif  // Shell_COMMON_KEY_弱_MAP_H_
