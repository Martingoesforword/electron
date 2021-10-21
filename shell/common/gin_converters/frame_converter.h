// 版权所有(C)2020 Samuel Maddock&lt;Sam@samuelmaddock.com&gt;。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_FRAME_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_FRAME_CONVERTER_H_

#include "gin/converter.h"
#include "shell/common/gin_helper/accessor.h"

namespace content {
class RenderFrameHost;
}

namespace gin {

template <>
struct Converter<content::RenderFrameHost*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::RenderFrameHost* val);
};

template <>
struct Converter<gin_helper::AccessorValue<content::RenderFrameHost*>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      gin_helper::AccessorValue<content::RenderFrameHost*> val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gin_helper::AccessorValue<content::RenderFrameHost*>* out);
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_CONVERTERS_FRAME_CONVERTER_H_
