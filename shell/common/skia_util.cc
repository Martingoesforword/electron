// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/pattern.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "net/base/data_url.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/node_includes.h"
#include "shell/common/skia_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPixelRef.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/image/image_util.h"

#if defined(OS_WIN)
#include "ui/gfx/icon_util.h"
#endif

namespace electron {

namespace util {

struct ScaleFactorPair {
  const char* name;
  float scale;
};

ScaleFactorPair kScaleFactorPairs[] = {
    // 为了提高比例尺匹配速度，我们把@2x放在第一位。
    {"@2x", 2.0f},   {"@3x", 3.0f},     {"@1x", 1.0f},     {"@4x", 4.0f},
    {"@5x", 5.0f},   {"@1.25x", 1.25f}, {"@1.33x", 1.33f}, {"@1.4x", 1.4f},
    {"@1.5x", 1.5f}, {"@1.8x", 1.8f},   {"@2.5x", 2.5f},
};

float GetScaleFactorFromPath(const base::FilePath& path) {
  std::string filename(path.BaseName().RemoveExtension().AsUTF8Unsafe());

  // 我们在这里不尝试将字符串转换为浮点型，因为它非常非常。
  // 很贵的。
  for (const auto& kScaleFactorPair : kScaleFactorPairs) {
    if (base::EndsWith(filename, kScaleFactorPair.name,
                       base::CompareCase::INSENSITIVE_ASCII))
      return kScaleFactorPair.scale;
  }

  return 1.0f;
}

bool AddImageSkiaRepFromPNG(gfx::ImageSkia* image,
                            const unsigned char* data,
                            size_t size,
                            double scale_factor) {
  SkBitmap bitmap;
  if (!gfx::PNGCodec::Decode(data, size, &bitmap))
    return false;

  image->AddRepresentation(gfx::ImageSkiaRep(bitmap, scale_factor));
  return true;
}

bool AddImageSkiaRepFromJPEG(gfx::ImageSkia* image,
                             const unsigned char* data,
                             size_t size,
                             double scale_factor) {
  auto bitmap = gfx::JPEGCodec::Decode(data, size);
  if (!bitmap)
    return false;

  // `JPEGCodec：：Decode()`不告诉它创建的`SkBitmap`实例。
  // 它的所有像素都是不透明的，这就是为什么位图。
  // Alpha类型为`kPremul_SkAlphaType`，而不是`kOpaque_SkAlphaType`。
  // 我们就在这里修吧。
  // TODO(Alexeykuzmin)：应删除此解决方法。
  // 当`JPEGCodec：：Decode()`代码固定时。
  // 请参阅https://github.com/electron/electron/issues/11294.。
  bitmap->setAlphaType(SkAlphaType::kOpaque_SkAlphaType);

  image->AddRepresentation(gfx::ImageSkiaRep(*bitmap, scale_factor));
  return true;
}

bool AddImageSkiaRepFromBuffer(gfx::ImageSkia* image,
                               const unsigned char* data,
                               size_t size,
                               int width,
                               int height,
                               double scale_factor) {
  // 先试试PNG。
  if (AddImageSkiaRepFromPNG(image, data, size, scale_factor))
    return true;

  // 第二次尝试JPEG。
  if (AddImageSkiaRepFromJPEG(image, data, size, scale_factor))
    return true;

  if (width == 0 || height == 0)
    return false;

  auto info = SkImageInfo::MakeN32(width, height, kPremul_SkAlphaType);
  if (size < info.computeMinByteSize())
    return false;

  SkBitmap bitmap;
  bitmap.allocN32Pixels(width, height, false);
  bitmap.writePixels({info, data, bitmap.rowBytes()});

  image->AddRepresentation(gfx::ImageSkiaRep(bitmap, scale_factor));
  return true;
}

bool AddImageSkiaRepFromPath(gfx::ImageSkia* image,
                             const base::FilePath& path,
                             double scale_factor) {
  std::string file_contents;
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    if (!asar::ReadFileToString(path, &file_contents))
      return false;
  }

  const auto* data =
      reinterpret_cast<const unsigned char*>(file_contents.data());
  size_t size = file_contents.size();

  return AddImageSkiaRepFromBuffer(image, data, size, 0, 0, scale_factor);
}

bool PopulateImageSkiaRepsFromPath(gfx::ImageSkia* image,
                                   const base::FilePath& path) {
  bool succeed = false;
  std::string filename(path.BaseName().RemoveExtension().AsUTF8Unsafe());
  if (base::MatchPattern(filename, "*@*x"))
    // 如果已指定DPI，则不要搜索其他表示形式。
    return AddImageSkiaRepFromPath(image, path, GetScaleFactorFromPath(path));
  else
    succeed |= AddImageSkiaRepFromPath(image, path, 1.0f);

  for (const ScaleFactorPair& pair : kScaleFactorPairs)
    succeed |= AddImageSkiaRepFromPath(
        image, path.InsertBeforeExtensionASCII(pair.name), pair.scale);
  return succeed;
}
#if defined(OS_WIN)
bool ReadImageSkiaFromICO(gfx::ImageSkia* image, HICON icon) {
  // 将图标从Windows特定的Hicon转换为gfx：：ImageSkia。
  SkBitmap bitmap = IconUtil::CreateSkBitmapFromHICON(icon);
  if (bitmap.isNull())
    return false;

  image->AddRepresentation(gfx::ImageSkiaRep(bitmap, 1.0f));
  return true;
}
#endif

}  // 命名空间实用程序。

}  // 命名空间电子
