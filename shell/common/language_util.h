// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_LANGUAGE_UTIL_H_
#define SHELL_COMMON_LANGUAGE_UTIL_H_

#include <string>
#include <vector>

namespace electron {

// 从操作系统返回用户首选语言列表。这份名单不包括。
// 从命令行参数重写。
std::vector<std::string> GetPreferredLanguages();

}  // 命名空间电子。

#endif  // Shell_COMMON_LANGUAGE_UTIL_H_
