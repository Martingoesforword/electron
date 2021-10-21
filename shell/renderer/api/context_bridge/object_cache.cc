// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/api/context_bridge/object_cache.h"

#include <utility>

#include "shell/common/api/object_life_monitor.h"

namespace electron {

namespace api {

namespace context_bridge {

ObjectCache::ObjectCache() = default;
ObjectCache::~ObjectCache() = default;

void ObjectCache::CacheProxiedObject(v8::Local<v8::Value> from,
                                     v8::Local<v8::Value> proxy_value) {
  if (from->IsObject() && !from->IsNullOrUndefined()) {
    auto obj = from.As<v8::Object>();
    int hash = obj->GetIdentityHash();

    proxy_map_[hash].push_front(std::make_pair(from, proxy_value));
  }
}

v8::MaybeLocal<v8::Value> ObjectCache::GetCachedProxiedObject(
    v8::Local<v8::Value> from) const {
  if (!from->IsObject() || from->IsNullOrUndefined())
    return v8::MaybeLocal<v8::Value>();

  auto obj = from.As<v8::Object>();
  int hash = obj->GetIdentityHash();
  auto iter = proxy_map_.find(hash);
  if (iter == proxy_map_.end())
    return v8::MaybeLocal<v8::Value>();

  auto& list = iter->second;
  for (const auto& pair : list) {
    auto from_cmp = pair.first;
    if (from_cmp == from) {
      if (pair.second.IsEmpty())
        return v8::MaybeLocal<v8::Value>();
      return pair.second;
    }
  }
  return v8::MaybeLocal<v8::Value>();
}

}  // 命名空间上下文桥接器。

}  // 命名空间API。

}  // 命名空间电子
