// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_PROCESS_UTIL_H_
#define SHELL_COMMON_PROCESS_UTIL_H_

#include <string>

namespace node {
class Environment;
}

namespace electron {

void EmitWarning(node::Environment* env,
                 const std::string& warning_msg,
                 const std::string& warning_type);

}  // 命名空间电子。

#endif  // Shell_COMMON_PROCESS_UTIL_H_
