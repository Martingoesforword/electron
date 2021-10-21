// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_PLATFORM_UTIL_H_
#define SHELL_COMMON_PLATFORM_UTIL_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "build/build_config.h"

class GURL;

namespace platform_util {

typedef base::OnceCallback<void(const std::string&)> OpenCallback;

// 在文件管理器中显示给定的文件。如果可能，请选择该文件。
// 必须从UI线程调用。
void ShowItemInFolder(const base::FilePath& full_path);

// 以桌面的默认方式打开给定的文件。
// 必须从UI线程调用。
void OpenPath(const base::FilePath& full_path, OpenCallback callback);

struct OpenExternalOptions {
  bool activate = true;
  base::FilePath working_dir;
};

// 以桌面的默认方式打开给定的外部协议URL。
// (例如，默认邮件用户代理中的mailto：URL。)。
void OpenExternal(const GURL& url,
                  const OpenExternalOptions& options,
                  OpenCallback callback);

// 异步将文件移至垃圾桶。
void TrashItem(const base::FilePath& full_path,
               base::OnceCallback<void(bool, const std::string&)> callback);

void Beep();

#if defined(OS_WIN)
// Chromium未涵盖的SHGetFolderPath调用。
bool GetFolderPath(int key, base::FilePath* result);
#endif

#if defined(OS_MAC)
bool GetLoginItemEnabled();
bool SetLoginItemEnabled(bool enabled);
#endif

#if defined(OS_LINUX)
// 返回成功标志。
// 与libgtkui不同，它“不”使用“Chrome-Browser.ktop”作为后备选项。
bool GetDesktopName(std::string* setme);
#endif

}  // 命名空间Platform_util。

#endif  // Shell_COMMON_Platform_Util_H_
