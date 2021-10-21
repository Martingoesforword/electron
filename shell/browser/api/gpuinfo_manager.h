// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_GPUINFO_MANAGER_H_
#define SHELL_BROWSER_API_GPUINFO_MANAGER_H_

#include <memory>
#include <vector>

#include "content/browser/gpu/gpu_data_manager_impl.h"  // 点名检查。
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/gpu_data_manager_observer.h"
#include "shell/common/gin_helper/promise.h"

namespace electron {

// GPUInfoManager是一个用于管理和获取GPUInfo的单例。
class GPUInfoManager : public content::GpuDataManagerObserver {
 public:
  static GPUInfoManager* GetInstance();

  GPUInfoManager();
  ~GPUInfoManager() override;
  bool NeedsCompleteGpuInfoCollection() const;
  void FetchCompleteInfo(gin_helper::Promise<base::DictionaryValue> promise);
  void FetchBasicInfo(gin_helper::Promise<base::DictionaryValue> promise);
  void OnGpuInfoUpdate() override;

 private:
  std::unique_ptr<base::DictionaryValue> EnumerateGPUInfo(
      gpu::GPUInfo gpu_info) const;

  // 这些内容应发布到任务队列。
  void CompleteInfoFetcher(gin_helper::Promise<base::DictionaryValue> promise);
  void ProcessCompleteInfo();

  // 这一套保留了所有应该兑现的承诺。
  // 一旦我们有了完整的信息数据。
  std::vector<gin_helper::Promise<base::DictionaryValue>>
      complete_info_promise_set_;
  content::GpuDataManagerImpl* gpu_data_manager_;

  DISALLOW_COPY_AND_ASSIGN(GPUInfoManager);
};

}  // 命名空间电子。
#endif  // Shell_Browser_API_GPUINFO_MANAGER_H_
