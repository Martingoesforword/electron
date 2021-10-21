// 版权所有(C)2021 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_LOGGING_H_
#define SHELL_COMMON_LOGGING_H_

namespace base {
class CommandLine;
class FilePath;
}  // 命名空间库。

namespace logging {

void InitElectronLogging(const base::CommandLine& command_line,
                         bool is_preinit);

base::FilePath GetLogFileName(const base::CommandLine& command_line);

}  // 命名空间日志记录。

#endif  // Shell_COMMON_LOGGING_H_
