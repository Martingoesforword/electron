// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_PLATFORM_UTIL_INTERNAL_H_
#define SHELL_COMMON_PLATFORM_UTIL_INTERNAL_H_

#include "shell/common/platform_util.h"

#include <string>

namespace base {
class FilePath;
}

namespace platform_util {
namespace internal {

// 由Platform_util.cc On调用以调用特定于平台的逻辑以移动。
// |path|使用合适的处理程序进行垃圾处理。
bool PlatformTrashItem(const base::FilePath& path, std::string* error);

}  // 命名空间内部。
}  // 命名空间Platform_util。

#endif  // Shell_COMMON_PLATFORM_UTIL_INTERNAL_H_
