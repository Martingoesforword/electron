// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/utility/electron_content_utility_client.h"

#include <utility>

#include "base/command_line.h"
#include "base/no_destructor.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/service_factory.h"
#include "sandbox/policy/switches.h"
#include "services/proxy_resolver/proxy_resolver_factory_impl.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/service.h"

#if defined(OS_WIN)
#include "chrome/services/util_win/public/mojom/util_read_icon.mojom.h"
#include "chrome/services/util_win/util_read_icon.h"
#endif  // 已定义(OS_WIN)。

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/services/print_compositor/print_compositor_impl.h"
#include "components/services/print_compositor/public/mojom/print_compositor.mojom.h"  // 点名检查。
#endif  // BUILDFLAG(ENABLE_PRINTING)。

#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
#include "chrome/services/printing/pdf_to_emf_converter_factory.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && defined(OS_WIN)
#include "chrome/utility/printing_handler.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN))
#include "chrome/services/printing/printing_service.h"
#include "chrome/services/printing/public/mojom/printing_service.mojom.h"
#endif

namespace electron {

namespace {

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN))
auto RunPrintingService(
    mojo::PendingReceiver<printing::mojom::PrintingService> receiver) {
  return std::make_unique<printing::PrintingService>(std::move(receiver));
}
#endif

#if defined(OS_WIN)
auto RunWindowsIconReader(
    mojo::PendingReceiver<chrome::mojom::UtilReadIcon> receiver) {
  return std::make_unique<UtilReadIcon>(std::move(receiver));
}
#endif

#if BUILDFLAG(ENABLE_PRINTING)
auto RunPrintCompositor(
    mojo::PendingReceiver<printing::mojom::PrintCompositor> receiver) {
  return std::make_unique<printing::PrintCompositorImpl>(
      std::move(receiver), true /* 初始化_环境。*/,
      content::UtilityThread::Get()->GetIOTaskRunner());
}
#endif

auto RunProxyResolver(
    mojo::PendingReceiver<proxy_resolver::mojom::ProxyResolverFactory>
        receiver) {
  return std::make_unique<proxy_resolver::ProxyResolverFactoryImpl>(
      std::move(receiver));
}

}  // 命名空间。

ElectronContentUtilityClient::ElectronContentUtilityClient() {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && defined(OS_WIN)
  printing_handler_ = std::make_unique<printing::PrintingHandler>();
#endif
}

ElectronContentUtilityClient::~ElectronContentUtilityClient() = default;

// 这一点的核心来自于铬的实现。
// Https://cs.chromium.org/chromium/src/chrome/utility/。
// Chrome_content_utility_client.cc?sq=package:chromium&dr=CSs&g=0&l=142。
void ElectronContentUtilityClient::ExposeInterfacesToBrowser(
    mojo::BinderMap* binders) {
#if defined(OS_WIN)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  utility_process_running_elevated_ = command_line->HasSwitch(
      sandbox::policy::switches::kNoSandboxAndElevatedPrivileges);
#endif

  // 如果我们的进程以提升的权限运行，则仅添加提升的Mojo。
  // 到BinderMap的接口。
  if (!utility_process_running_elevated_) {
#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
    binders->Add(
        base::BindRepeating(printing::PdfToEmfConverterFactory::Create),
        base::ThreadTaskRunnerHandle::Get());
#endif
  }
}

void ElectronContentUtilityClient::RegisterMainThreadServices(
    mojo::ServiceFactory& services) {
#if defined(OS_WIN)
  services.Add(RunWindowsIconReader);
#endif

#if BUILDFLAG(ENABLE_PRINTING)
  services.Add(RunPrintCompositor);
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN))
  services.Add(RunPrintingService);
#endif
}

void ElectronContentUtilityClient::RegisterIOThreadServices(
    mojo::ServiceFactory& services) {
  services.Add(RunProxyResolver);
}

}  // 命名空间电子
