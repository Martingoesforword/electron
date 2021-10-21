// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_IMAGE_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_IMAGE_CONVERTER_H_

#include "gin/converter.h"

namespace gfx {
class Image;
class ImageSkia;
}  // 命名空间gfx。

namespace gin {

template <>
struct Converter<gfx::ImageSkia> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::ImageSkia* out);
};

template <>
struct Converter<gfx::Image> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Image* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const gfx::Image& val);
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_CONVERTERS_IMAGE_CONVERTER_H_
