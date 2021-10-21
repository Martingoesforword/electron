// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_
#define SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_

#include <map>
#include <string>
#include "base/files/file_path.h"

namespace electron {

namespace api {

namespace crash_reporter {

bool IsCrashReporterEnabled();

#if defined(OS_LINUX)
const std::map<std::string, std::string>& GetGlobalCrashKeys();
std::string GetClientId();
#endif

// JS绑定API；公开，因为它也是从node_main.cc调用的。
void Start(const std::string& submit_url,
           bool upload_to_server,
           bool ignore_system_crash_handler,
           bool rate_limit,
           bool compress,
           const std::map<std::string, std::string>& global_extra,
           const std::map<std::string, std::string>& extra,
           bool is_node_process);

}  // 命名空间CRASH_REPORTER。

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_
