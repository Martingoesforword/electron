// 版权所有2015年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_ELECTRON_PATHS_H_
#define SHELL_COMMON_ELECTRON_PATHS_H_

#include "base/base_paths.h"

#if defined(OS_WIN)
#include "base/base_paths_win.h"
#elif defined(OS_MAC)
#include "base/base_paths_mac.h"
#endif

#if defined(OS_POSIX)
#include "base/base_paths_posix.h"
#endif

namespace electron {

enum {
  PATH_START = 11000,

  DIR_USER_CACHE = PATH_START,  // 可以写入用户缓存的目录。
  DIR_APP_LOGS,                 // 应用程序日志所在的目录。

#if defined(OS_WIN)
  DIR_RECENT,  // 最近文件所在的目录。
#endif

#if defined(OS_LINUX)
  DIR_APP_DATA,  // 用户配置文件下的应用程序数据目录。
#endif

  DIR_CRASH_DUMPS,  // C.F.。Chrome：：DIR_CRASH_DUMP。

  PATH_END,  // 新路径的终点。后面的内容重定向到base：：dir_*。

#if !defined(OS_LINUX)
  DIR_APP_DATA = base::DIR_APP_DATA,
#endif
};

static_assert(PATH_START < PATH_END, "invalid PATH boundaries");

}  // 命名空间电子。

#endif  // 壳层公共电子路径H_
