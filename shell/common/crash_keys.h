// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_CRASH_KEYS_H_
#define SHELL_COMMON_CRASH_KEYS_H_

#include <map>
#include <string>

namespace base {
class CommandLine;
}

namespace electron {

namespace crash_keys {

void SetCrashKey(const std::string& key, const std::string& value);
void ClearCrashKey(const std::string& key);
void GetCrashKeys(std::map<std::string, std::string>* keys);

void SetCrashKeysFromCommandLine(const base::CommandLine& command_line);
void SetPlatformCrashKey();

}  // 命名空间CRASH_KEYS。

}  // 命名空间电子。

#endif  // SHELL_COMMON_CRASH_KEYS_H_
