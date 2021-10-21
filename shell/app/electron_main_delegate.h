// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_APP_ELECTRON_MAIN_DELEGATE_H_
#define SHELL_APP_ELECTRON_MAIN_DELEGATE_H_

#include <memory>
#include <string>

#include "content/public/app/content_main_delegate.h"
#include "content/public/common/content_client.h"

namespace tracing {
class TracingSamplerProfiler;
}

namespace electron {

std::string LoadResourceBundle(const std::string& locale);

class ElectronMainDelegate : public content::ContentMainDelegate {
 public:
  static const char* const kNonWildcardDomainNonPortSchemes[];
  static const size_t kNonWildcardDomainNonPortSchemesSize;
  ElectronMainDelegate();
  ~ElectronMainDelegate() override;

 protected:
  // 内容：：ContentMainDelegate：
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  void SandboxInitialized(const std::string& process_type) override;
  void PreBrowserMain() override;
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentGpuClient* CreateContentGpuClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;
  content::ContentUtilityClient* CreateContentUtilityClient() override;
  int RunProcess(
      const std::string& process_type,
      const content::MainFunctionParams& main_function_params) override;
  bool ShouldCreateFeatureList() override;
  bool ShouldLockSchemeRegistry() override;
#if defined(OS_LINUX)
  void ZygoteForked() override;
#endif

 private:
#if defined(OS_MAC)
  void OverrideChildProcessPath();
  void OverrideFrameworkBundlePath();
  void SetUpBundleOverrides();
#endif

  std::unique_ptr<content::ContentBrowserClient> browser_client_;
  std::unique_ptr<content::ContentClient> content_client_;
  std::unique_ptr<content::ContentGpuClient> gpu_client_;
  std::unique_ptr<content::ContentRendererClient> renderer_client_;
  std::unique_ptr<content::ContentUtilityClient> utility_client_;
  std::unique_ptr<tracing::TracingSamplerProfiler> tracing_sampler_profiler_;

  DISALLOW_COPY_AND_ASSIGN(ElectronMainDelegate);
};

}  // 命名空间电子。

#endif  // Shell_APP_ELEMENT_Main_Delegate_H_
