// 版权所有(C)2020 Microsoft，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_BLUETOOTH_ELECTRON_BLUETOOTH_DELEGATE_H_
#define SHELL_BROWSER_BLUETOOTH_ELECTRON_BLUETOOTH_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "content/public/browser/bluetooth_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "third_party/blink/public/mojom/bluetooth/web_bluetooth.mojom-forward.h"

namespace blink {
class WebBluetoothDeviceId;
}

namespace content {
class RenderFrameHost;
}

namespace device {
class BluetoothDevice;
class BluetoothUUID;
}  // 命名空间设备。

namespace electron {

// 提供用于管理Web蓝牙的设备权限的界面，并。
// Web蓝牙扫描API。这是特定于电子设备的实现。
// 蓝图代表。
class ElectronBluetoothDelegate : public content::BluetoothDelegate {
 public:
  ElectronBluetoothDelegate();
  ~ElectronBluetoothDelegate() override;

  // 仅限行动的班级。
  ElectronBluetoothDelegate(const ElectronBluetoothDelegate&) = delete;
  ElectronBluetoothDelegate& operator=(const ElectronBluetoothDelegate&) =
      delete;

  // BluetothDelegate实现：
  std::unique_ptr<content::BluetoothChooser> RunBluetoothChooser(
      content::RenderFrameHost* frame,
      const content::BluetoothChooser::EventHandler& event_handler) override;

  std::unique_ptr<content::BluetoothScanningPrompt> ShowBluetoothScanningPrompt(
      content::RenderFrameHost* frame,
      const content::BluetoothScanningPrompt::EventHandler& event_handler)
      override;
  blink::WebBluetoothDeviceId GetWebBluetoothDeviceId(
      content::RenderFrameHost* frame,
      const std::string& device_address) override;
  std::string GetDeviceAddress(
      content::RenderFrameHost* frame,
      const blink::WebBluetoothDeviceId& device_id) override;
  blink::WebBluetoothDeviceId AddScannedDevice(
      content::RenderFrameHost* frame,
      const std::string& device_address) override;
  blink::WebBluetoothDeviceId GrantServiceAccessPermission(
      content::RenderFrameHost* frame,
      const device::BluetoothDevice* device,
      const blink::mojom::WebBluetoothRequestDeviceOptions* options) override;
  bool HasDevicePermission(
      content::RenderFrameHost* frame,
      const blink::WebBluetoothDeviceId& device_id) override;
  bool IsAllowedToAccessService(content::RenderFrameHost* frame,
                                const blink::WebBluetoothDeviceId& device_id,
                                const device::BluetoothUUID& service) override;
  bool IsAllowedToAccessAtLeastOneService(
      content::RenderFrameHost* frame,
      const blink::WebBluetoothDeviceId& device_id) override;
  bool IsAllowedToAccessManufacturerData(
      content::RenderFrameHost* frame,
      const blink::WebBluetoothDeviceId& device_id,
      uint16_t manufacturer_code) override;
  std::vector<blink::mojom::WebBluetoothDevicePtr> GetPermittedDevices(
      content::RenderFrameHost* frame) override;
  void AddFramePermissionObserver(FramePermissionObserver* observer) override;
  void RemoveFramePermissionObserver(
      FramePermissionObserver* observer) override;
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_BLUETOOTH_ELECTRON_BLUETOOTH_DELEGATE_H_
