// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_API_CONTEXT_BRIDGE_OBJECT_CACHE_H_
#define SHELL_RENDERER_API_CONTEXT_BRIDGE_OBJECT_CACHE_H_

#include <forward_list>
#include <unordered_map>
#include <utility>

#include "base/containers/linked_list.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace api {

namespace context_bridge {

using ObjectCachePair = std::pair<v8::Local<v8::Value>, v8::Local<v8::Value>>;

class ObjectCache final {
 public:
  ObjectCache();
  ~ObjectCache();

  void CacheProxiedObject(v8::Local<v8::Value> from,
                          v8::Local<v8::Value> proxy_value);
  v8::MaybeLocal<v8::Value> GetCachedProxiedObject(
      v8::Local<v8::Value> from) const;

 private:
  // Object_Identity==&gt;[FROM_VALUE，PROXY_VALUE]。
  std::unordered_map<int, std::forward_list<ObjectCachePair>> proxy_map_;
};

}  // 命名空间上下文桥接器。

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_RENDERER_API_CONTEXT_BRIDGE_OBJECT_CACHE_H_
