// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_APP_ELECTRON_LIBRARY_MAIN_H_
#define SHELL_APP_ELECTRON_LIBRARY_MAIN_H_

#include "build/build_config.h"
#include "electron/buildflags/buildflags.h"

#if defined(OS_MAC)
extern "C" {
__attribute__((visibility("default"))) int ElectronMain(int argc, char* argv[]);

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
__attribute__((visibility("default"))) int ElectronInitializeICUandStartNode(
    int argc,
    char* argv[]);
#endif
}
#endif  // OS_MAC。

#endif  // Shell_APP_ELECT_LIBRARY_Main_H_
