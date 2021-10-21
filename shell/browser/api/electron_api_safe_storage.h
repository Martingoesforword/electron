// 版权所有(C)2021 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_
#define SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_

#include "base/dcheck_is_on.h"

namespace electron {

namespace safestorage {

// 在DCHECK中用于验证我们假设的网络上下文。
// 在应用程序就绪保持为真之前，管理器已初始化。仅用于。
// 测试版本。
#if DCHECK_IS_ON()
void SetElectronCryptoReady(bool ready);
#endif

}  // 命名空间安全存储。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_SAFE_STORAGE_H_
