// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/color_util.h"

#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace electron {

SkColor ParseHexColor(const std::string& color_string) {
  // 检查字符串的格式是否不正确。
  if (color_string.empty() || color_string[0] != '#')
    return SK_ColorWHITE;

  // 如果未指定Alpha通道，则前置FF。
  std::string source = color_string.substr(1);
  if (source.size() == 3)
    source.insert(0, "F");
  else if (source.size() == 6)
    source.insert(0, "FF");

  // 将字符串从#FFF格式转换为#FFFFFF格式。
  std::string formatted_color;
  if (source.size() == 4) {
    for (size_t i = 0; i < 4; ++i) {
      formatted_color += source[i];
      formatted_color += source[i];
    }
  } else if (source.size() == 8) {
    formatted_color = source;
  } else {
    return SK_ColorWHITE;
  }

  // 将字符串转换为整数并确保其值正确。
  // 射程。
  std::vector<uint8_t> bytes;
  if (!base::HexStringToBytes(formatted_color, &bytes))
    return SK_ColorWHITE;

  return SkColorSetARGB(bytes[0], bytes[1], bytes[2], bytes[3]);
}

std::string ToRGBHex(SkColor color) {
  return base::StringPrintf("#%02X%02X%02X", SkColorGetR(color),
                            SkColorGetG(color), SkColorGetB(color));
}

std::string ToRGBAHex(SkColor color, bool include_hash) {
  std::string color_str = base::StringPrintf(
      "%02X%02X%02X%02X", SkColorGetR(color), SkColorGetG(color),
      SkColorGetB(color), SkColorGetA(color));
  if (include_hash) {
    return "#" + color_str;
  }
  return color_str;
}

}  // 命名空间电子
