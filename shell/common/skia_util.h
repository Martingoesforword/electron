// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_SKIA_UTIL_H_
#define SHELL_COMMON_SKIA_UTIL_H_

namespace base {
class FilePath;
}

namespace gfx {
class ImageSkia;
}

namespace electron {

namespace util {

bool PopulateImageSkiaRepsFromPath(gfx::ImageSkia* image,
                                   const base::FilePath& path);

bool AddImageSkiaRepFromBuffer(gfx::ImageSkia* image,
                               const unsigned char* data,
                               size_t size,
                               int width,
                               int height,
                               double scale_factor);

bool AddImageSkiaRepFromJPEG(gfx::ImageSkia* image,
                             const unsigned char* data,
                             size_t size,
                             double scale_factor);

bool AddImageSkiaRepFromPNG(gfx::ImageSkia* image,
                            const unsigned char* data,
                            size_t size,
                            double scale_factor);

#if defined(OS_WIN)
bool ReadImageSkiaFromICO(gfx::ImageSkia* image, HICON icon);
#endif

}  // 命名空间实用程序。

}  // 命名空间电子。

#endif  // Shell_COMMON_SKIA_UTIL_H_
