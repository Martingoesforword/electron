// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_GFX_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_GFX_CONVERTER_H_

#include "gin/converter.h"

namespace display {
class Display;
}

namespace gfx {
class Point;
class PointF;
class Size;
class Rect;
enum class ResizeEdge;
}  // 命名空间gfx。

namespace gin {

template <>
struct Converter<gfx::Point> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const gfx::Point& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Point* out);
};

template <>
struct Converter<gfx::PointF> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const gfx::PointF& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::PointF* out);
};

template <>
struct Converter<gfx::Size> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const gfx::Size& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Size* out);
};

template <>
struct Converter<gfx::Rect> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const gfx::Rect& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Rect* out);
};

template <>
struct Converter<display::Display> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const display::Display& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     display::Display* out);
};

template <>
struct Converter<gfx::ResizeEdge> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const gfx::ResizeEdge& val);
};

}  // 命名空间杜松子酒。

#endif  // Shell_COMMON_GIN_CONVERTERS_GFX_CONFERTER_H_
