// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_KEYBOARD_UTIL_H_
#define SHELL_COMMON_KEYBOARD_UTIL_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace electron {

// 返回字符的按键代码，同时判断Shift键是否。
// 熨好了。
ui::KeyboardCode KeyboardCodeFromCharCode(char16_t c, bool* shifted);

// 返回|str|的键码，如果原键为移位字符，
// 例如+和/，将其设置在|SHIFT_CHAR|中。
// 熨好了。
ui::KeyboardCode KeyboardCodeFromStr(const std::string& str,
                                     absl::optional<char16_t>* shifted_char);

}  // 命名空间电子。

#endif  // Shell_COMMON_COMPAY_UTIL_H_
