// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/hid/hid_chooser_context.h"

#include <utility>

#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/device_service.h"
#include "electron/grit/electron_resources.h"
#include "services/device/public/cpp/hid/hid_blocklist.h"
#include "services/device/public/cpp/hid/hid_switches.h"
#include "shell/browser/web_contents_permission_helper.h"
#include "ui/base/l10n/l10n_util.h"

namespace electron {

const char kHidDeviceNameKey[] = "name";
const char kHidGuidKey[] = "guid";
const char kHidVendorIdKey[] = "vendorId";
const char kHidProductIdKey[] = "productId";
const char kHidSerialNumberKey[] = "serialNumber";

HidChooserContext::HidChooserContext(ElectronBrowserContext* context)
    : browser_context_(context) {}

HidChooserContext::~HidChooserContext() {
  // 通知观察者选择器上下文即将被销毁。
  // 观察员必须将自己从观察员名单中删除。
  for (auto& observer : device_observer_list_) {
    observer.OnHidChooserContextShutdown();
    DCHECK(!device_observer_list_.HasObserver(&observer));
  }
}

// 静电。
std::u16string HidChooserContext::DisplayNameFromDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  if (device.product_name.empty()) {
    auto device_id_string = base::ASCIIToUTF16(
        base::StringPrintf("%04X:%04X", device.vendor_id, device.product_id));
    return l10n_util::GetStringFUTF16(IDS_HID_CHOOSER_ITEM_WITHOUT_NAME,
                                      device_id_string);
  }
  return base::UTF8ToUTF16(device.product_name);
}

// 静电。
bool HidChooserContext::CanStorePersistentEntry(
    const device::mojom::HidDeviceInfo& device) {
  return !device.serial_number.empty() && !device.product_name.empty();
}

// 静电。
base::Value HidChooserContext::DeviceInfoToValue(
    const device::mojom::HidDeviceInfo& device) {
  base::Value value(base::Value::Type::DICTIONARY);
  value.SetStringKey(
      kHidDeviceNameKey,
      base::UTF16ToUTF8(HidChooserContext::DisplayNameFromDeviceInfo(device)));
  value.SetIntKey(kHidVendorIdKey, device.vendor_id);
  value.SetIntKey(kHidProductIdKey, device.product_id);
  if (HidChooserContext::CanStorePersistentEntry(device)) {
    // 使用USB序列号作为永久标识符。如果是的话。
    // 不可用，只能授予短暂权限。
    value.SetStringKey(kHidSerialNumberKey, device.serial_number);
  } else {
    // GUID是在连接时创建的临时ID，在。
    // 设备已断开连接。临时权限以此ID为关键字。
    // 并且必须在每次连接设备时再次授予。
    value.SetStringKey(kHidGuidKey, device.guid);
  }
  return value;
}

void HidChooserContext::GrantDevicePermission(
    const url::Origin& origin,
    const device::mojom::HidDeviceInfo& device,
    content::RenderFrameHost* render_frame_host) {
  DCHECK(base::Contains(devices_, device.guid));
  if (CanStorePersistentEntry(device)) {
    auto* web_contents =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    auto* permission_helper =
        WebContentsPermissionHelper::FromWebContents(web_contents);
    permission_helper->GrantHIDDevicePermission(
        origin, DeviceInfoToValue(device), render_frame_host);
  } else {
    ephemeral_devices_[origin].insert(device.guid);
  }
}

bool HidChooserContext::HasDevicePermission(
    const url::Origin& origin,
    const device::mojom::HidDeviceInfo& device,
    content::RenderFrameHost* render_frame_host) {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableHidBlocklist) &&
      device::HidBlocklist::IsDeviceExcluded(device))
    return false;

  auto it = ephemeral_devices_.find(origin);
  if (it != ephemeral_devices_.end() &&
      base::Contains(it->second, device.guid)) {
    return true;
  }

  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  return permission_helper->CheckHIDDevicePermission(
      origin, DeviceInfoToValue(device), render_frame_host);
}

void HidChooserContext::AddDeviceObserver(DeviceObserver* observer) {
  EnsureHidManagerConnection();
  device_observer_list_.AddObserver(observer);
}

void HidChooserContext::RemoveDeviceObserver(DeviceObserver* observer) {
  device_observer_list_.RemoveObserver(observer);
}

void HidChooserContext::GetDevices(
    device::mojom::HidManager::GetDevicesCallback callback) {
  if (!is_initialized_) {
    EnsureHidManagerConnection();
    pending_get_devices_requests_.push(std::move(callback));
    return;
  }

  std::vector<device::mojom::HidDeviceInfoPtr> device_list;
  device_list.reserve(devices_.size());
  for (const auto& pair : devices_)
    device_list.push_back(pair.second->Clone());
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(device_list)));
}

const device::mojom::HidDeviceInfo* HidChooserContext::GetDeviceInfo(
    const std::string& guid) {
  DCHECK(is_initialized_);
  auto it = devices_.find(guid);
  return it == devices_.end() ? nullptr : it->second.get();
}

device::mojom::HidManager* HidChooserContext::GetHidManager() {
  EnsureHidManagerConnection();
  return hid_manager_.get();
}

base::WeakPtr<HidChooserContext> HidChooserContext::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void HidChooserContext::DeviceAdded(device::mojom::HidDeviceInfoPtr device) {
  DCHECK(device);

  // 更新设备列表。
  if (!base::Contains(devices_, device->guid))
    devices_.insert({device->guid, device->Clone()});

  // 通知所有观察员。
  for (auto& observer : device_observer_list_)
    observer.OnDeviceAdded(*device);
}

void HidChooserContext::DeviceRemoved(device::mojom::HidDeviceInfoPtr device) {
  DCHECK(device);
  DCHECK(base::Contains(devices_, device->guid));

  // 更新设备列表。
  devices_.erase(device->guid);

  // 通知所有设备观察者。
  for (auto& observer : device_observer_list_)
    observer.OnDeviceRemoved(*device);

  // 接下来，我们将通知观察者撤销的权限。如果设备没有。
  // 支持永久权限，则设备权限将在。
  // 断开连接。
  if (CanStorePersistentEntry(*device))
    return;

  std::vector<url::Origin> revoked_origins;
  for (auto& map_entry : ephemeral_devices_) {
    if (map_entry.second.erase(device->guid) > 0)
      revoked_origins.push_back(map_entry.first);
  }
  if (revoked_origins.empty())
    return;
}

void HidChooserContext::DeviceChanged(device::mojom::HidDeviceInfoPtr device) {
  DCHECK(device);
  DCHECK(base::Contains(devices_, device->guid));

  // 更新设备列表。
  devices_[device->guid] = device->Clone();

  // 通知所有观察员。
  for (auto& observer : device_observer_list_)
    observer.OnDeviceChanged(*device);
}

void HidChooserContext::EnsureHidManagerConnection() {
  if (hid_manager_)
    return;

  mojo::PendingRemote<device::mojom::HidManager> manager;
  content::GetDeviceService().BindHidManager(
      manager.InitWithNewPipeAndPassReceiver());
  SetUpHidManagerConnection(std::move(manager));
}

void HidChooserContext::SetUpHidManagerConnection(
    mojo::PendingRemote<device::mojom::HidManager> manager) {
  hid_manager_.Bind(std::move(manager));
  hid_manager_.set_disconnect_handler(base::BindOnce(
      &HidChooserContext::OnHidManagerConnectionError, base::Unretained(this)));

  hid_manager_->GetDevicesAndSetClient(
      client_receiver_.BindNewEndpointAndPassRemote(),
      base::BindOnce(&HidChooserContext::InitDeviceList,
                     weak_factory_.GetWeakPtr()));
}

void HidChooserContext::InitDeviceList(
    std::vector<device::mojom::HidDeviceInfoPtr> devices) {
  for (auto& device : devices)
    devices_.insert({device->guid, std::move(device)});

  is_initialized_ = true;

  while (!pending_get_devices_requests_.empty()) {
    std::vector<device::mojom::HidDeviceInfoPtr> device_list;
    device_list.reserve(devices.size());
    for (const auto& entry : devices_)
      device_list.push_back(entry.second->Clone());
    std::move(pending_get_devices_requests_.front())
        .Run(std::move(device_list));
    pending_get_devices_requests_.pop();
  }
}

void HidChooserContext::OnHidManagerConnectionError() {
  hid_manager_.reset();
  client_receiver_.reset();
  devices_.clear();

  std::vector<url::Origin> revoked_origins;
  revoked_origins.reserve(ephemeral_devices_.size());
  for (const auto& map_entry : ephemeral_devices_)
    revoked_origins.push_back(map_entry.first);
  ephemeral_devices_.clear();

  // 通知所有设备观察者。
  for (auto& observer : device_observer_list_)
    observer.OnHidManagerConnectionError();
}

}  // 命名空间电子
