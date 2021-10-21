// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_APPLICATION_INFO_H_
#define SHELL_COMMON_APPLICATION_INFO_H_

#if defined(OS_WIN)
#include "shell/browser/win/scoped_hstring.h"
#endif

#include <string>

namespace electron {

std::string& OverriddenApplicationName();
std::string& OverriddenApplicationVersion();

std::string GetPossiblyOverriddenApplicationName();

std::string GetApplicationName();
std::string GetApplicationVersion();
// 返回Electron的用户代理。
std::string GetApplicationUserAgent();

#if defined(OS_WIN)
PCWSTR GetRawAppUserModelID();
bool GetAppUserModelID(ScopedHString* app_id);
void SetAppUserModelID(const std::wstring& name);
bool IsRunningInDesktopBridge();
#endif

}  // 命名空间电子。

#endif  // Shell_COMMON_APPLICATION_INFO_H_
