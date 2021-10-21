// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/gpuinfo_manager.h"

#include <utility>

#include "base/memory/singleton.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "gpu/config/gpu_info_collector.h"
#include "shell/browser/api/gpu_info_enumerator.h"
#include "shell/common/gin_converters/value_converter.h"

namespace electron {

GPUInfoManager* GPUInfoManager::GetInstance() {
  return base::Singleton<GPUInfoManager>::get();
}

GPUInfoManager::GPUInfoManager()
    : gpu_data_manager_(content::GpuDataManagerImpl::GetInstance()) {
  gpu_data_manager_->AddObserver(this);
}

GPUInfoManager::~GPUInfoManager() {
  content::GpuDataManagerImpl::GetInstance()->RemoveObserver(this);
}

// 基于。
// Https://chromium.googlesource.com/chromium/src.git/+/69.0.3497.106/content/browser/gpu/gpu_data_manager_impl_private.cc#838。
bool GPUInfoManager::NeedsCompleteGpuInfoCollection() const {
#if defined(OS_WIN)
  return gpu_data_manager_->DxdiagDx12VulkanRequested() &&
         gpu_data_manager_->GetGPUInfo().dx_diagnostics.IsEmpty();
#else
  return false;
#endif
}

// 应该发布到任务运行者。
void GPUInfoManager::ProcessCompleteInfo() {
  const auto result = EnumerateGPUInfo(gpu_data_manager_->GetGPUInfo());
  // 我们已经收到了完整的信息，解决了所有的承诺。
  // 都在等这条消息。
  for (auto& promise : complete_info_promise_set_) {
    promise.Resolve(*result);
  }
  complete_info_promise_set_.clear();
}

void GPUInfoManager::OnGpuInfoUpdate() {
  // 如果未要求完整的GPUInfo时调用，则忽略。
  if (NeedsCompleteGpuInfoCollection())
    return;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&GPUInfoManager::ProcessCompleteInfo,
                                base::Unretained(this)));
}

// 应该发布到任务运行者。
void GPUInfoManager::CompleteInfoFetcher(
    gin_helper::Promise<base::DictionaryValue> promise) {
  complete_info_promise_set_.emplace_back(std::move(promise));

  if (NeedsCompleteGpuInfoCollection()) {
    gpu_data_manager_->RequestDxdiagDx12VulkanVideoGpuInfoIfNeeded(
        content::GpuDataManagerImpl::kGpuInfoRequestAll, /* 延迟。*/ false);
  } else {
    GPUInfoManager::OnGpuInfoUpdate();
  }
}

void GPUInfoManager::FetchCompleteInfo(
    gin_helper::Promise<base::DictionaryValue> promise) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&GPUInfoManager::CompleteInfoFetcher,
                                base::Unretained(this), std::move(promise)));
}

// 这会同步获取信息，因此不需要发布到任务队列。
// 不能有多个承诺，因为它们是同步解析的。
void GPUInfoManager::FetchBasicInfo(
    gin_helper::Promise<base::DictionaryValue> promise) {
  gpu::GPUInfo gpu_info;
  CollectBasicGraphicsInfo(&gpu_info);
  promise.Resolve(*EnumerateGPUInfo(gpu_info));
}

std::unique_ptr<base::DictionaryValue> GPUInfoManager::EnumerateGPUInfo(
    gpu::GPUInfo gpu_info) const {
  GPUInfoEnumerator enumerator;
  gpu_info.EnumerateFields(&enumerator);
  return enumerator.GetDictionary();
}

}  // 命名空间电子
