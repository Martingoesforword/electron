// 版权所有(C)2021 Microsoft，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_HID_HID_CHOOSER_CONTROLLER_H_
#define SHELL_BROWSER_HID_HID_CHOOSER_CONTROLLER_H_

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/hid_chooser.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "services/device/public/mojom/hid.mojom-forward.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/hid/electron_hid_delegate.h"
#include "shell/browser/hid/hid_chooser_context.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "third_party/blink/public/mojom/hid/hid.mojom.h"

namespace content {
class RenderFrameHost;
}  // 命名空间内容。

namespace electron {

class ElectronHidDelegate;

class HidChooserContext;

// HidChooserController为WebHID API权限提示提供数据。
class HidChooserController
    : public content::WebContentsObserver,
      public electron::HidChooserContext::DeviceObserver {
 public:
  // 构建人机界面设备(HID)的选择器控制器。
  // |RENDER_FRAME_HOST|用于初始化选择器字符串并访问。
  // 请求来源和嵌入来源。|callback|选择器时调用。
  // 通过选择一项或关闭选择器对话框来关闭。
  // 使用所选设备调用回调，如果没有设备，则调用nullptr。
  // 被选中了。
  HidChooserController(content::RenderFrameHost* render_frame_host,
                       std::vector<blink::mojom::HidDeviceFilterPtr> filters,
                       content::HidChooser::Callback callback,
                       content::WebContents* web_contents,
                       base::WeakPtr<ElectronHidDelegate> hid_delegate);
  HidChooserController(HidChooserController&) = delete;
  HidChooserController& operator=(HidChooserController&) = delete;
  ~HidChooserController() override;

  // HidChooserContext：：DeviceViewer：
  void OnDeviceAdded(const device::mojom::HidDeviceInfo& device_info) override;
  void OnDeviceRemoved(
      const device::mojom::HidDeviceInfo& device_info) override;
  void OnDeviceChanged(
      const device::mojom::HidDeviceInfo& device_info) override;
  void OnHidManagerConnectionError() override;
  void OnHidChooserContextShutdown() override;

  // 内容：：WebContentsViewer：
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;

 private:
  api::Session* GetSession();
  void OnGotDevices(std::vector<device::mojom::HidDeviceInfoPtr> devices);
  bool DisplayDevice(const device::mojom::HidDeviceInfo& device) const;
  bool FilterMatchesAny(const device::mojom::HidDeviceInfo& device) const;

  // 将|DEVICE_INFO|添加到|DEVICE_MAP_|。该设备将添加到选择器项目中。
  // 表示物理设备。如果选择器项尚不存在，则会引发。
  // 将追加新项目。如果追加了项，则返回TRUE。
  bool AddDeviceInfo(const device::mojom::HidDeviceInfo& device_info);

  // 从|DEVICE_MAP_|中删除|DEVICE_INFO|。设备信息将从。
  // 表示物理设备的选择器项。如果这会导致。
  // 项为空，则删除选择器项。如果该设备未执行任何操作，则不执行任何操作。
  // 不在选择器项中。如果项已删除，则返回TRUE。
  bool RemoveDeviceInfo(const device::mojom::HidDeviceInfo& device_info);

  // 更新|DEVICE_INFO|中|DEVICE_INFO|描述的设备的信息。
  // |device_map_|。
  void UpdateDeviceInfo(const device::mojom::HidDeviceInfo& device_info);

  void RunCallback(std::vector<device::mojom::HidDeviceInfoPtr> devices);
  void OnDeviceChosen(gin::Arguments* args);

  std::vector<blink::mojom::HidDeviceFilterPtr> filters_;
  content::HidChooser::Callback callback_;
  const url::Origin origin_;
  const int frame_tree_node_id_;

  // 选择器上下文的生存期与用于。
  // 创建它，并且可以在选择器仍处于活动状态时将其销毁。
  base::WeakPtr<HidChooserContext> chooser_context_;

  // 有关连接的设备及其HID接口的信息。单曲。
  // 物理设备可能暴露多个HID接口。密钥是物理密钥。
  // 设备ID，值是HidDeviceInfo对象的集合，表示。
  // 物理设备托管的HID接口。
  std::map<std::string, std::vector<device::mojom::HidDeviceInfoPtr>>
      device_map_;

  // 确定项目顺序的物理设备ID的有序列表。
  // 在选择器里。
  std::vector<std::string> items_;

  base::ScopedObservation<HidChooserContext,
                          HidChooserContext::DeviceObserver,
                          &HidChooserContext::AddDeviceObserver,
                          &HidChooserContext::RemoveDeviceObserver>
      observation_{this};

  base::WeakPtr<ElectronHidDelegate> hid_delegate_;

  content::GlobalRenderFrameHostId render_frame_host_id_;

  base::WeakPtrFactory<HidChooserController> weak_factory_{this};
};

}  // 命名空间电子。

#endif  // Shell_Browser_HID_HID_Chooser_Controller_H_
