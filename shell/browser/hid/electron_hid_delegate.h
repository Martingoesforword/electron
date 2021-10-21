// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_HID_ELECTRON_HID_DELEGATE_H_
#define SHELL_BROWSER_HID_ELECTRON_HID_DELEGATE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "content/public/browser/hid_delegate.h"
#include "shell/browser/hid/hid_chooser_context.h"

namespace electron {

class HidChooserController;

class ElectronHidDelegate : public content::HidDelegate,
                            public HidChooserContext::DeviceObserver {
 public:
  ElectronHidDelegate();
  ElectronHidDelegate(ElectronHidDelegate&) = delete;
  ElectronHidDelegate& operator=(ElectronHidDelegate&) = delete;
  ~ElectronHidDelegate() override;

  // 内容：：HidDelegate：
  std::unique_ptr<content::HidChooser> RunChooser(
      content::RenderFrameHost* render_frame_host,
      std::vector<blink::mojom::HidDeviceFilterPtr> filters,
      content::HidChooser::Callback callback) override;
  bool CanRequestDevicePermission(
      content::RenderFrameHost* render_frame_host) override;
  bool HasDevicePermission(content::RenderFrameHost* render_frame_host,
                           const device::mojom::HidDeviceInfo& device) override;
  device::mojom::HidManager* GetHidManager(
      content::RenderFrameHost* render_frame_host) override;
  void AddObserver(content::RenderFrameHost* render_frame_host,
                   content::HidDelegate::Observer* observer) override;
  void RemoveObserver(content::RenderFrameHost* render_frame_host,
                      content::HidDelegate::Observer* observer) override;
  const device::mojom::HidDeviceInfo* GetDeviceInfo(
      content::RenderFrameHost* render_frame_host,
      const std::string& guid) override;
  bool IsFidoAllowedForOrigin(const url::Origin& origin) override;

  // HidChooserContext：：DeviceViewer：
  void OnDeviceAdded(const device::mojom::HidDeviceInfo&) override;
  void OnDeviceRemoved(const device::mojom::HidDeviceInfo&) override;
  void OnDeviceChanged(const device::mojom::HidDeviceInfo&) override;
  void OnHidManagerConnectionError() override;
  void OnHidChooserContextShutdown() override;

  void DeleteControllerForFrame(content::RenderFrameHost* render_frame_host);

 private:
  HidChooserController* ControllerForFrame(
      content::RenderFrameHost* render_frame_host);

  HidChooserController* AddControllerForFrame(
      content::RenderFrameHost* render_frame_host,
      std::vector<blink::mojom::HidDeviceFilterPtr> filters,
      content::HidChooser::Callback callback);

  base::ScopedObservation<HidChooserContext,
                          HidChooserContext::DeviceObserver,
                          &HidChooserContext::AddDeviceObserver,
                          &HidChooserContext::RemoveDeviceObserver>
      device_observation_{this};
  base::ObserverList<content::HidDelegate::Observer> observer_list_;

  std::unordered_map<content::RenderFrameHost*,
                     std::unique_ptr<HidChooserController>>
      controller_map_;

  base::WeakPtrFactory<ElectronHidDelegate> weak_factory_{this};
};

}  // 命名空间电子。

#endif  // Shell_Browser_HID_Electronics_HID_Delegate_H_
