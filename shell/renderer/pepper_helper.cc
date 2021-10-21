// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/pepper_helper.h"

#include <memory>

#include "chrome/renderer/pepper/chrome_renderer_pepper_host_factory.h"
#include "chrome/renderer/pepper/pepper_shared_memory_message_filter.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "electron/shell/renderer/electron_renderer_pepper_host_factory.h"
#include "ppapi/host/ppapi_host.h"

PepperHelper::PepperHelper(content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {}

PepperHelper::~PepperHelper() = default;

void PepperHelper::DidCreatePepperPlugin(content::RendererPpapiHost* host) {
  // TODO(Brettw)弄清楚如何连接主机工厂。它需要一些。
  // 一种类似过滤器的系统，允许动态添加。
  host->GetPpapiHost()->AddHostFactoryFilter(
      std::make_unique<ChromeRendererPepperHostFactory>(host));
  host->GetPpapiHost()->AddHostFactoryFilter(
      std::make_unique<ElectronRendererPepperHostFactory>(host));
  host->GetPpapiHost()->AddInstanceMessageFilter(
      std::make_unique<PepperSharedMemoryMessageFilter>(host));
}

void PepperHelper::OnDestruct() {
  delete this;
}
