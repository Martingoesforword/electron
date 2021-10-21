// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/electron_gpu_client.h"

#include "base/environment.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace electron {

ElectronGpuClient::ElectronGpuClient() = default;

void ElectronGpuClient::PreCreateMessageLoop() {
#if defined(OS_WIN)
  auto env = base::Environment::Create();
  if (env->HasVar("ELECTRON_DEFAULT_ERROR_MODE"))
    SetErrorMode(GetErrorMode() & ~SEM_NOGPFAULTERRORBOX);
#endif
}

}  // 命名空间电子
