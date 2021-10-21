// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/serial/serial_chooser_context.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/containers/contains.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/device_service.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "shell/browser/web_contents_permission_helper.h"

namespace electron {

constexpr char kPortNameKey[] = "name";
constexpr char kTokenKey[] = "token";

#if defined(OS_WIN)
const char kDeviceInstanceIdKey[] = "device_instance_id";
#else
const char kVendorIdKey[] = "vendor_id";
const char kProductIdKey[] = "product_id";
const char kSerialNumberKey[] = "serial_number";
#if defined(OS_MAC)
const char kUsbDriverKey[] = "usb_driver";
#endif  // 已定义(OS_MAC)。
#endif  // 已定义(OS_WIN。

std::string EncodeToken(const base::UnguessableToken& token) {
  const uint64_t data[2] = {token.GetHighForSerialization(),
                            token.GetLowForSerialization()};
  std::string buffer;
  base::Base64Encode(
      base::StringPiece(reinterpret_cast<const char*>(&data[0]), sizeof(data)),
      &buffer);
  return buffer;
}

base::UnguessableToken DecodeToken(base::StringPiece input) {
  std::string buffer;
  if (!base::Base64Decode(input, &buffer) ||
      buffer.length() != sizeof(uint64_t) * 2) {
    return base::UnguessableToken();
  }

  const uint64_t* data = reinterpret_cast<const uint64_t*>(buffer.data());
  return base::UnguessableToken::Deserialize(data[0], data[1]);
}

base::Value PortInfoToValue(const device::mojom::SerialPortInfo& port) {
  base::Value value(base::Value::Type::DICTIONARY);
  if (port.display_name && !port.display_name->empty())
    value.SetStringKey(kPortNameKey, *port.display_name);
  else
    value.SetStringKey(kPortNameKey, port.path.LossyDisplayName());

  if (!SerialChooserContext::CanStorePersistentEntry(port)) {
    value.SetStringKey(kTokenKey, EncodeToken(port.token));
    return value;
  }

#if defined(OS_WIN)
  // Windows提供了一个方便的设备标识符，我们可以依赖它。
  // 足够稳定，可在重启后识别设备。
  value.SetStringKey(kDeviceInstanceIdKey, port.device_instance_id);
#else
  DCHECK(port.has_vendor_id);
  value.SetIntKey(kVendorIdKey, port.vendor_id);
  DCHECK(port.has_product_id);
  value.SetIntKey(kProductIdKey, port.product_id);
  DCHECK(port.serial_number);
  value.SetStringKey(kSerialNumberKey, *port.serial_number);

#if defined(OS_MAC)
  DCHECK(port.usb_driver_name && !port.usb_driver_name->empty());
  value.SetStringKey(kUsbDriverKey, *port.usb_driver_name);
#endif  // 已定义(OS_MAC)。
#endif  // 已定义(OS_WIN)。
  return value;
}

SerialChooserContext::SerialChooserContext() = default;

SerialChooserContext::~SerialChooserContext() = default;

void SerialChooserContext::GrantPortPermission(
    const url::Origin& origin,
    const device::mojom::SerialPortInfo& port,
    content::RenderFrameHost* render_frame_host) {
  base::Value value = PortInfoToValue(port);
  port_info_.insert({port.token, value.Clone()});

  if (CanStorePersistentEntry(port)) {
    auto* web_contents =
        content::WebContents::FromRenderFrameHost(render_frame_host);
    auto* permission_helper =
        WebContentsPermissionHelper::FromWebContents(web_contents);
    permission_helper->GrantSerialPortPermission(origin, std::move(value),
                                                 render_frame_host);
    return;
  }

  ephemeral_ports_[origin].insert(port.token);
}

bool SerialChooserContext::HasPortPermission(
    const url::Origin& origin,
    const device::mojom::SerialPortInfo& port,
    content::RenderFrameHost* render_frame_host) {
  auto it = ephemeral_ports_.find(origin);
  if (it != ephemeral_ports_.end()) {
    const std::set<base::UnguessableToken> ports = it->second;
    if (base::Contains(ports, port.token))
      return true;
  }

  if (!CanStorePersistentEntry(port)) {
    return false;
  }

  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  base::Value value = PortInfoToValue(port);
  return permission_helper->CheckSerialPortPermission(origin, std::move(value),
                                                      render_frame_host);
}

// 静电。
bool SerialChooserContext::CanStorePersistentEntry(
    const device::mojom::SerialPortInfo& port) {
  // 如果没有显示名称，则将使用路径名。这个。
  // 不能保证路径名是稳定的。例如，在Linux上，名称。
  // “ttyUSB0”可重复用于任何USB串行设备。这样的名字应该是。
  // 当设备断开连接时，在设置中显示令人困惑。
  if (!port.display_name || port.display_name->empty())
    return false;

#if defined(OS_WIN)
  return !port.device_instance_id.empty();
#else
  if (!port.has_vendor_id || !port.has_product_id || !port.serial_number ||
      port.serial_number->empty()) {
    return false;
  }

#if defined(OS_MAC)
  // 标准USB供应商ID、产品ID和序列号的组合。
  // 数字属性应足以唯一标识设备。
  // 不过，最新版本的MacOS包含内置驱动程序。
  // USB转串行适配器的类型，而它们的制造商仍然。
  // 建议安装他们的自定义驱动程序。当两个都加载了两个。
  // 找到每个设备的IOSerialBSDClient实例。包括。
  // USB驱动程序名称使我们可以区分这两者。
  if (!port.usb_driver_name || port.usb_driver_name->empty())
    return false;
#endif  // 已定义(OS_MAC)。

  return true;
#endif  // 已定义(OS_WIN)。
}

device::mojom::SerialPortManager* SerialChooserContext::GetPortManager() {
  EnsurePortManagerConnection();
  return port_manager_.get();
}

void SerialChooserContext::AddPortObserver(PortObserver* observer) {
  port_observer_list_.AddObserver(observer);
}

void SerialChooserContext::RemovePortObserver(PortObserver* observer) {
  port_observer_list_.RemoveObserver(observer);
}

base::WeakPtr<SerialChooserContext> SerialChooserContext::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void SerialChooserContext::OnPortAdded(device::mojom::SerialPortInfoPtr port) {
  for (auto& observer : port_observer_list_)
    observer.OnPortAdded(*port);
}

void SerialChooserContext::OnPortRemoved(
    device::mojom::SerialPortInfoPtr port) {
  for (auto& observer : port_observer_list_)
    observer.OnPortRemoved(*port);

  port_info_.erase(port->token);
}

void SerialChooserContext::EnsurePortManagerConnection() {
  if (port_manager_)
    return;

  mojo::PendingRemote<device::mojom::SerialPortManager> manager;
  content::GetDeviceService().BindSerialPortManager(
      manager.InitWithNewPipeAndPassReceiver());
  SetUpPortManagerConnection(std::move(manager));
}

void SerialChooserContext::SetUpPortManagerConnection(
    mojo::PendingRemote<device::mojom::SerialPortManager> manager) {
  port_manager_.Bind(std::move(manager));
  port_manager_.set_disconnect_handler(
      base::BindOnce(&SerialChooserContext::OnPortManagerConnectionError,
                     base::Unretained(this)));

  port_manager_->SetClient(client_receiver_.BindNewPipeAndPassRemote());
}

void SerialChooserContext::OnPortManagerConnectionError() {
  port_manager_.reset();
  client_receiver_.reset();

  port_info_.clear();

  ephemeral_ports_.clear();
}

}  // 命名空间电子
