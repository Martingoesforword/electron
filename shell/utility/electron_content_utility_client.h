// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_UTILITY_ELECTRON_CONTENT_UTILITY_CLIENT_H_
#define SHELL_UTILITY_ELECTRON_CONTENT_UTILITY_CLIENT_H_

#include <memory>

#include "base/compiler_specific.h"
#include "content/public/utility/content_utility_client.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "printing/buildflags/buildflags.h"

namespace printing {
class PrintingHandler;
}

namespace mojo {
class ServiceFactory;
}

namespace electron {

class ElectronContentUtilityClient : public content::ContentUtilityClient {
 public:
  ElectronContentUtilityClient();
  ~ElectronContentUtilityClient() override;

  void ExposeInterfacesToBrowser(mojo::BinderMap* binders) override;
  void RegisterMainThreadServices(mojo::ServiceFactory& services) override;
  void RegisterIOThreadServices(mojo::ServiceFactory& services) override;

 private:
#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && defined(OS_WIN)
  std::unique_ptr<printing::PrintingHandler> printing_handler_;
#endif

  // 如果实用程序进程以提升的权限运行，则为True。
  bool utility_process_running_elevated_ = false;

  DISALLOW_COPY_AND_ASSIGN(ElectronContentUtilityClient);
};

}  // 命名空间电子。

#endif  // SHELL_UTILITY_ELECTRON_CONTENT_UTILITY_CLIENT_H_
