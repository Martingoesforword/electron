// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_COLOR_UTIL_H_
#define SHELL_COMMON_COLOR_UTIL_H_

#include <string>

#include "third_party/skia/include/core/SkColor.h"

namespace electron {

// 分析十六进制颜色，如“#FFF”或“#EFEFEF”
SkColor ParseHexColor(const std::string& color_string);

// 将颜色转换为RGB十六进制值，如“#ABCDEF”
std::string ToRGBHex(SkColor color);

std::string ToRGBAHex(SkColor color, bool include_hash = true);

}  // 命名空间电子。

#endif  // Shell_COMMON_COLOR_UTIL_H_
