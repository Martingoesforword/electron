// 版权所有(C)2021 Microsoft，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_H_
#define SHELL_BROWSER_HID_HID_CHOOSER_CONTEXT_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/queue.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/unguessable_token.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "shell/browser/electron_browser_context.h"
#include "url/origin.h"

namespace base {
class Value;
}

namespace electron {

extern const char kHidDeviceNameKey[];
extern const char kHidGuidKey[];
extern const char kHidVendorIdKey[];
extern const char kHidProductIdKey[];
extern const char kHidSerialNumberKey[];

// 控件的内部状态和到设备服务的连接。
// 人机接口设备(HID)选择器UI。
class HidChooserContext : public KeyedService,
                          public device::mojom::HidManagerClient {
 public:
  // 此观察器可用于在HID设备已连接或。
  // 已断开连接。
  class DeviceObserver : public base::CheckedObserver {
   public:
    virtual void OnDeviceAdded(const device::mojom::HidDeviceInfo&) = 0;
    virtual void OnDeviceRemoved(const device::mojom::HidDeviceInfo&) = 0;
    virtual void OnDeviceChanged(const device::mojom::HidDeviceInfo&) = 0;
    virtual void OnHidManagerConnectionError() = 0;

    // 在HidChooserContext关闭时调用。观察员必须移除。
    // 在回来之前。
    virtual void OnHidChooserContextShutdown() = 0;
  };

  explicit HidChooserContext(ElectronBrowserContext* context);
  HidChooserContext(const HidChooserContext&) = delete;
  HidChooserContext& operator=(const HidChooserContext&) = delete;
  ~HidChooserContext() override;

  // 返回|device|的人类可读的字符串标识符。
  static std::u16string DisplayNameFromDeviceInfo(
      const device::mojom::HidDeviceInfo& device);

  // 如果可以为|device|授予持久权限，则返回TRUE。
  static bool CanStorePersistentEntry(
      const device::mojom::HidDeviceInfo& device);

  static base::Value DeviceInfoToValue(
      const device::mojom::HidDeviceInfo& device);

  // 用于授予和检查权限的HID特定接口。
  void GrantDevicePermission(const url::Origin& origin,
                             const device::mojom::HidDeviceInfo& device,
                             content::RenderFrameHost* render_frame_host);
  bool HasDevicePermission(const url::Origin& origin,
                           const device::mojom::HidDeviceInfo& device,
                           content::RenderFrameHost* render_frame_host);

  // 用于作用域观察者。
  void AddDeviceObserver(DeviceObserver* observer);
  void RemoveDeviceObserver(DeviceObserver* observer);

  // 转发HidManager：：GetDevices。
  void GetDevices(device::mojom::HidManager::GetDevicesCallback callback);

  // 只有在确定|DEVICES_|已预先初始化的情况下才调用此函数。
  // 返回的原始指针归|DEVICES_|所有，在发生以下情况时将被销毁。
  // 该设备将被移除。
  const device::mojom::HidDeviceInfo* GetDeviceInfo(const std::string& guid);

  device::mojom::HidManager* GetHidManager();

  base::WeakPtr<HidChooserContext> AsWeakPtr();

 private:
  // Device：：mojom：：HidManagerClient实现：
  void DeviceAdded(device::mojom::HidDeviceInfoPtr device_info) override;
  void DeviceRemoved(device::mojom::HidDeviceInfoPtr device_info) override;
  void DeviceChanged(device::mojom::HidDeviceInfoPtr device_info) override;

  void EnsureHidManagerConnection();
  void SetUpHidManagerConnection(
      mojo::PendingRemote<device::mojom::HidManager> manager);
  void InitDeviceList(std::vector<device::mojom::HidDeviceInfoPtr> devices);
  void OnHidManagerInitializedForTesting(
      device::mojom::HidManager::GetDevicesCallback callback,
      std::vector<device::mojom::HidDeviceInfoPtr> devices);
  void OnHidManagerConnectionError();

  ElectronBrowserContext* browser_context_;

  bool is_initialized_ = false;
  base::queue<device::mojom::HidManager::GetDevicesCallback>
      pending_get_devices_requests_;

  // 跟踪源有权访问的设备集。
  std::map<url::Origin, std::set<std::string>> ephemeral_devices_;

  // 从设备GUID映射到设备信息。
  std::map<std::string, device::mojom::HidDeviceInfoPtr> devices_;

  mojo::Remote<device::mojom::HidManager> hid_manager_;
  mojo::AssociatedReceiver<device::mojom::HidManagerClient> client_receiver_{
      this};
  base::ObserverList<DeviceObserver> device_observer_list_;

  base::WeakPtrFactory<HidChooserContext> weak_factory_{this};
};

}  // 命名空间电子。

#endif  // Shell_Browser_HID_HID_Chooser_Context_H_
