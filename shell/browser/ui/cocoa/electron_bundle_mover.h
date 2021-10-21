// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_ELECTRON_BUNDLE_MOVER_H_
#define SHELL_BROWSER_UI_COCOA_ELECTRON_BUNDLE_MOVER_H_

#include <string>

#include "base/mac/foundation_util.h"
#include "shell/common/gin_helper/error_thrower.h"

namespace gin {
class Arguments;
}

namespace electron {

// 可能的捆绑包移动冲突。
enum class BundlerMoverConflictType { kExists, kExistsAndRunning };

class ElectronBundleMover {
 public:
  static bool Move(gin_helper::ErrorThrower thrower, gin::Arguments* args);
  static bool IsCurrentAppInApplicationsFolder();

 private:
  static bool ShouldContinueMove(gin_helper::ErrorThrower thrower,
                                 BundlerMoverConflictType type,
                                 gin::Arguments* args);
  static bool IsInApplicationsFolder(NSString* bundlePath);
  static NSString* ContainingDiskImageDevice(NSString* bundlePath);
  static void Relaunch(NSString* destinationPath);
  static NSString* ShellQuotedString(NSString* string);
  static bool CopyBundle(NSString* srcPath, NSString* dstPath);
  static bool AuthorizedInstall(NSString* srcPath,
                                NSString* dstPath,
                                bool* canceled);
  static bool IsApplicationAtPathRunning(NSString* bundlePath);
  static bool DeleteOrTrash(NSString* path);
  static bool Trash(NSString* path);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_COCOA_ELECTRON_BUNDLE_MOVER_H_
