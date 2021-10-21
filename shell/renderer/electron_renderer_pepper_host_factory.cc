// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/electron_renderer_pepper_host_factory.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "content/public/renderer/renderer_ppapi_host.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/ppapi_message_utils.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/ppapi_permissions.h"

using ppapi::host::ResourceHost;

// 忽略所有消息的存根类。
class PepperUMAHost : public ppapi::host::ResourceHost {
 public:
  PepperUMAHost(content::RendererPpapiHost* host,
                PP_Instance instance,
                PP_Resource resource)
      : ResourceHost(host->GetPpapiHost(), instance, resource) {}
  ~PepperUMAHost() override = default;

  // PPAPI：：Host：：ResourceMessageHandler实现。
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override {
    PPAPI_BEGIN_MESSAGE_MAP(PepperUMAHost, msg)
      PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UMA_HistogramCustomTimes,
                                        OnHistogramCustomTimes)
      PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UMA_HistogramCustomCounts,
                                        OnHistogramCustomCounts)
      PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UMA_HistogramEnumeration,
                                        OnHistogramEnumeration)
      PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
          PpapiHostMsg_UMA_IsCrashReportingEnabled, OnIsCrashReportingEnabled)
    PPAPI_END_MESSAGE_MAP()
    return PP_ERROR_FAILED;
  }

 private:
  int32_t OnHistogramCustomTimes(ppapi::host::HostMessageContext* context,
                                 const std::string& name,
                                 int64_t sample,
                                 int64_t min,
                                 int64_t max,
                                 uint32_t bucket_count) {
    return PP_OK;
  }

  int32_t OnHistogramCustomCounts(ppapi::host::HostMessageContext* context,
                                  const std::string& name,
                                  int32_t sample,
                                  int32_t min,
                                  int32_t max,
                                  uint32_t bucket_count) {
    return PP_OK;
  }

  int32_t OnHistogramEnumeration(ppapi::host::HostMessageContext* context,
                                 const std::string& name,
                                 int32_t sample,
                                 int32_t boundary_value) {
    return PP_OK;
  }

  int32_t OnIsCrashReportingEnabled(ppapi::host::HostMessageContext* context) {
    return PP_OK;
  }
};

ElectronRendererPepperHostFactory::ElectronRendererPepperHostFactory(
    content::RendererPpapiHost* host)
    : host_(host) {}

ElectronRendererPepperHostFactory::~ElectronRendererPepperHostFactory() =
    default;

std::unique_ptr<ResourceHost>
ElectronRendererPepperHostFactory::CreateResourceHost(
    ppapi::host::PpapiHost* host,
    PP_Resource resource,
    PP_Instance instance,
    const IPC::Message& message) {
  DCHECK_EQ(host_->GetPpapiHost(), host);

  // 确保插件为我们提供了此资源的有效实例。
  if (!host_->IsValidInstance(instance))
    return nullptr;

  // 将在以下位置检查以下接口的权限。
  // 相应实例的方法调用的时间。目前这些。
  // 界面仅对特定许可的应用程序可用，这些应用程序可能。
  // 无法访问其他专用接口。
  switch (message.type()) {
    case PpapiHostMsg_UMA_Create::ID: {
      return std::make_unique<PepperUMAHost>(host_, instance, resource);
    }
  }

  return nullptr;
}
