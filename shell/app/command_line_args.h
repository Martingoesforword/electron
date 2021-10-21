// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_APP_COMMAND_LINE_ARGS_H_
#define SHELL_APP_COMMAND_LINE_ARGS_H_

#include "base/command_line.h"

namespace electron {

bool CheckCommandLineArguments(int argc, base::CommandLine::CharType** argv);
bool IsSandboxEnabled(base::CommandLine* command_line);

}  // 命名空间电子。

#endif  // SHELL_APP_COMMAND_LINE_ARGS_H_
