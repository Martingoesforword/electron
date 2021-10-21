// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_ELECTRON_RENDERER_PEPPER_HOST_FACTORY_H_
#define SHELL_RENDERER_ELECTRON_RENDERER_PEPPER_HOST_FACTORY_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ppapi/host/host_factory.h"

namespace content {
class RendererPpapiHost;
}

class ElectronRendererPepperHostFactory : public ppapi::host::HostFactory {
 public:
  explicit ElectronRendererPepperHostFactory(content::RendererPpapiHost* host);
  ~ElectronRendererPepperHostFactory() override;

  // 主机工厂。
  std::unique_ptr<ppapi::host::ResourceHost> CreateResourceHost(
      ppapi::host::PpapiHost* host,
      PP_Resource resource,
      PP_Instance instance,
      const IPC::Message& message) override;

 private:
  // 不属于此对象。
  content::RendererPpapiHost* host_;

  DISALLOW_COPY_AND_ASSIGN(ElectronRendererPepperHostFactory);
};

#endif  // SHELL_RENDERER_ELECTRON_RENDERER_PEPPER_HOST_FACTORY_H_
