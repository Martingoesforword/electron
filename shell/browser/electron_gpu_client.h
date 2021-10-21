// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_GPU_CLIENT_H_
#define SHELL_BROWSER_ELECTRON_GPU_CLIENT_H_

#include "content/public/gpu/content_gpu_client.h"

namespace electron {

class ElectronGpuClient : public content::ContentGpuClient {
 public:
  ElectronGpuClient();

  // 内容：：ContentGpuClient：
  void PreCreateMessageLoop() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronGpuClient);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Electronics_GPU_Client_H_
