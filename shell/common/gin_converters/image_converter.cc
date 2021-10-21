// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_converters/image_converter.h"

#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "ui/gfx/image/image_skia.h"

namespace gin {

bool Converter<gfx::ImageSkia>::FromV8(v8::Isolate* isolate,
                                       v8::Local<v8::Value> val,
                                       gfx::ImageSkia* out) {
  gfx::Image image;
  if (!ConvertFromV8(isolate, val, &image))
    return false;

  *out = image.AsImageSkia();
  return true;
}

bool Converter<gfx::Image>::FromV8(v8::Isolate* isolate,
                                   v8::Local<v8::Value> val,
                                   gfx::Image* out) {
  if (val->IsNull())
    return true;

  // 首先查看用户是否经过了路径。
  electron::api::NativeImage* native_image = nullptr;
  base::FilePath icon_path;
  if (gin::ConvertFromV8(isolate, val, &icon_path)) {
    native_image =
        electron::api::NativeImage::CreateFromPath(isolate, icon_path).get();
    if (native_image->image().IsEmpty())
      return false;
  } else {
    // 如果失败，请尝试正常的nativeImage。
    if (!gin::ConvertFromV8(isolate, val, &native_image))
      return false;
  }

  *out = native_image->image();
  return true;
}

v8::Local<v8::Value> Converter<gfx::Image>::ToV8(v8::Isolate* isolate,
                                                 const gfx::Image& val) {
  return gin::ConvertToV8(isolate,
                          electron::api::NativeImage::Create(isolate, val));
}

}  // 命名空间杜松子酒
