// 版权所有(C)2021 Microsoft，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/hid/hid_chooser_controller.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/ranges/algorithm.h"
#include "base/stl_util.h"
#include "gin/data_object_builder.h"
#include "services/device/public/cpp/hid/hid_blocklist.h"
#include "services/device/public/cpp/hid/hid_switches.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/hid/hid_chooser_context.h"
#include "shell/browser/hid/hid_chooser_context_factory.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/process_util.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

std::string PhysicalDeviceIdFromDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  // 单个物理设备可以公开多个HID接口，每个接口。
  // 由HidDeviceInfo对象表示。当设备公开多个。
  // HID接口，则HidDeviceInfo对象将共享。
  // |Physical_Device_id|。对这些设备进行分组，以便单个选择器项目。
  // 为每台物理设备显示。如果设备的物理设备ID是。
  // 空，请改用其GUID。
  return device.physical_device_id.empty() ? device.guid
                                           : device.physical_device_id;
}

}  // 命名空间。

namespace gin {

template <>
struct Converter<device::mojom::HidDeviceInfoPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const device::mojom::HidDeviceInfoPtr& device) {
    base::Value value = electron::HidChooserContext::DeviceInfoToValue(*device);
    value.SetStringKey("deviceId", PhysicalDeviceIdFromDeviceInfo(*device));
    return gin::ConvertToV8(isolate, value);
  }
};

}  // 命名空间杜松子酒。

namespace electron {

HidChooserController::HidChooserController(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::HidDeviceFilterPtr> filters,
    content::HidChooser::Callback callback,
    content::WebContents* web_contents,
    base::WeakPtr<ElectronHidDelegate> hid_delegate)
    : WebContentsObserver(web_contents),
      filters_(std::move(filters)),
      callback_(std::move(callback)),
      origin_(content::WebContents::FromRenderFrameHost(render_frame_host)
                  ->GetMainFrame()
                  ->GetLastCommittedOrigin()),
      frame_tree_node_id_(render_frame_host->GetFrameTreeNodeId()),
      hid_delegate_(hid_delegate),
      render_frame_host_id_(render_frame_host->GetGlobalId()) {
  chooser_context_ = HidChooserContextFactory::GetForBrowserContext(
                         web_contents->GetBrowserContext())
                         ->AsWeakPtr();
  DCHECK(chooser_context_);

  chooser_context_->GetHidManager()->GetDevices(base::BindOnce(
      &HidChooserController::OnGotDevices, weak_factory_.GetWeakPtr()));
}

HidChooserController::~HidChooserController() {
  if (callback_)
    std::move(callback_).Run(std::vector<device::mojom::HidDeviceInfoPtr>());
}

api::Session* HidChooserController::GetSession() {
  if (!web_contents()) {
    return nullptr;
  }
  return api::Session::FromBrowserContext(web_contents()->GetBrowserContext());
}

void HidChooserController::OnDeviceAdded(
    const device::mojom::HidDeviceInfo& device) {
  if (!DisplayDevice(device))
    return;
  if (AddDeviceInfo(device)) {
    api::Session* session = GetSession();
    if (session) {
      auto* rfh = content::RenderFrameHost::FromID(render_frame_host_id_);
      v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
      v8::HandleScope scope(isolate);
      v8::Local<v8::Object> details = gin::DataObjectBuilder(isolate)
                                          .Set("device", device.Clone())
                                          .Set("frame", rfh)
                                          .Build();
      session->Emit("hid-device-added", details);
    }
  }

  return;
}

void HidChooserController::OnDeviceRemoved(
    const device::mojom::HidDeviceInfo& device) {
  auto id = PhysicalDeviceIdFromDeviceInfo(device);
  auto items_it = std::find(items_.begin(), items_.end(), id);
  if (items_it == items_.end())
    return;
  api::Session* session = GetSession();
  if (session) {
    auto* rfh = content::RenderFrameHost::FromID(render_frame_host_id_);
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Object> details = gin::DataObjectBuilder(isolate)
                                        .Set("device", device.Clone())
                                        .Set("frame", rfh)
                                        .Build();
    session->Emit("hid-device-removed", details);
  }
  RemoveDeviceInfo(device);
}

void HidChooserController::OnDeviceChanged(
    const device::mojom::HidDeviceInfo& device) {
  bool has_chooser_item =
      base::Contains(items_, PhysicalDeviceIdFromDeviceInfo(device));
  if (!DisplayDevice(device)) {
    if (has_chooser_item)
      OnDeviceRemoved(device);
    return;
  }

  if (!has_chooser_item) {
    OnDeviceAdded(device);
    return;
  }

  // 更新项目以将旧设备信息替换为|Device|。
  UpdateDeviceInfo(device);
}

void HidChooserController::OnDeviceChosen(gin::Arguments* args) {
  std::string device_id;
  if (!args->GetNext(&device_id) || device_id.empty()) {
    RunCallback({});
  } else {
    auto find_it = device_map_.find(device_id);
    if (find_it != device_map_.end()) {
      auto& device_infos = find_it->second;
      std::vector<device::mojom::HidDeviceInfoPtr> devices;
      devices.reserve(device_infos.size());
      for (auto& device : device_infos) {
        chooser_context_->GrantDevicePermission(origin_, *device,
                                                web_contents()->GetMainFrame());
        devices.push_back(device->Clone());
      }
      RunCallback(std::move(devices));
    } else {
      v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
      node::Environment* env = node::Environment::GetCurrent(isolate);
      EmitWarning(env, "The device id " + device_id + " was not found.",
                  "UnknownHIDDeviceId");
      RunCallback({});
    }
  }
}

void HidChooserController::OnHidManagerConnectionError() {
  observation_.Reset();
}

void HidChooserController::OnHidChooserContextShutdown() {
  observation_.Reset();
}

void HidChooserController::OnGotDevices(
    std::vector<device::mojom::HidDeviceInfoPtr> devices) {
  std::vector<device::mojom::HidDeviceInfoPtr> devicesToDisplay;
  devicesToDisplay.reserve(devices.size());

  for (auto& device : devices) {
    if (DisplayDevice(*device)) {
      if (AddDeviceInfo(*device)) {
        devicesToDisplay.push_back(device->Clone());
      }
    }
  }

  // 监听OnDeviceAdded/Remove事件的HidChooserContext。
  // 枚举。
  if (chooser_context_)
    observation_.Observe(chooser_context_.get());
  bool prevent_default = false;
  api::Session* session = GetSession();
  if (session) {
    auto* rfh = content::RenderFrameHost::FromID(render_frame_host_id_);
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Object> details = gin::DataObjectBuilder(isolate)
                                        .Set("deviceList", devicesToDisplay)
                                        .Set("frame", rfh)
                                        .Build();
    prevent_default =
        session->Emit("select-hid-device", details,
                      base::AdaptCallbackForRepeating(
                          base::BindOnce(&HidChooserController::OnDeviceChosen,
                                         weak_factory_.GetWeakPtr())));
  }
  if (!prevent_default) {
    RunCallback({});
  }
}

bool HidChooserController::DisplayDevice(
    const device::mojom::HidDeviceInfo& device) const {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableHidBlocklist)) {
    // 如果设备被阻止列表排除，请不要将其传递给选择器。
    if (device::HidBlocklist::IsDeviceExcluded(device))
      return false;

    // 如果设备具有顶级集合，请不要将其传递给选择器。
    // 使用FIDO使用情况页面。
    // 
    // 注意：HID阻止列表也会阻止FIDO的顶级收集。
    // 使用情况页面，但如果设备有其他(非FIDO)，则不会阻止该设备。
    // 收藏品。下面的检查将从选择器中排除该设备。
    // 如果它有任何顶级的FIDO集合。
    auto find_it =
        std::find_if(device.collections.begin(), device.collections.end(),
                     [](const device::mojom::HidCollectionInfoPtr& c) {
                       return c->usage->usage_page == device::mojom::kPageFido;
                     });
    if (find_it != device.collections.end())
      return false;
  }

  return FilterMatchesAny(device);
}

bool HidChooserController::FilterMatchesAny(
    const device::mojom::HidDeviceInfo& device) const {
  if (filters_.empty())
    return true;

  for (const auto& filter : filters_) {
    if (filter->device_ids) {
      if (filter->device_ids->is_vendor()) {
        if (filter->device_ids->get_vendor() != device.vendor_id)
          continue;
      } else if (filter->device_ids->is_vendor_and_product()) {
        const auto& vendor_and_product =
            filter->device_ids->get_vendor_and_product();
        if (vendor_and_product->vendor != device.vendor_id)
          continue;
        if (vendor_and_product->product != device.product_id)
          continue;
      }
    }

    if (filter->usage) {
      if (filter->usage->is_page()) {
        const uint16_t usage_page = filter->usage->get_page();
        auto find_it =
            std::find_if(device.collections.begin(), device.collections.end(),
                         [=](const device::mojom::HidCollectionInfoPtr& c) {
                           return usage_page == c->usage->usage_page;
                         });
        if (find_it == device.collections.end())
          continue;
      } else if (filter->usage->is_usage_and_page()) {
        const auto& usage_and_page = filter->usage->get_usage_and_page();
        auto find_it = std::find_if(
            device.collections.begin(), device.collections.end(),
            [&usage_and_page](const device::mojom::HidCollectionInfoPtr& c) {
              return usage_and_page->usage_page == c->usage->usage_page &&
                     usage_and_page->usage == c->usage->usage;
            });
        if (find_it == device.collections.end())
          continue;
      }
    }

    return true;
  }

  return false;
}

bool HidChooserController::AddDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  auto id = PhysicalDeviceIdFromDeviceInfo(device);
  auto find_it = device_map_.find(id);
  if (find_it != device_map_.end()) {
    find_it->second.push_back(device.Clone());
    return false;
  }
  // 连接了一个新设备。将其追加到选择器列表的末尾。
  device_map_[id].push_back(device.Clone());
  items_.push_back(id);
  return true;
}

bool HidChooserController::RemoveDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  auto id = PhysicalDeviceIdFromDeviceInfo(device);
  auto find_it = device_map_.find(id);
  DCHECK(find_it != device_map_.end());
  auto& device_infos = find_it->second;
  base::EraseIf(device_infos,
                [&device](const device::mojom::HidDeviceInfoPtr& d) {
                  return d->guid == device.guid;
                });
  if (!device_infos.empty())
    return false;
  // 设备已断开连接。将其从选择器列表中移除。
  device_map_.erase(find_it);
  base::Erase(items_, id);
  return true;
}

void HidChooserController::UpdateDeviceInfo(
    const device::mojom::HidDeviceInfo& device) {
  auto id = PhysicalDeviceIdFromDeviceInfo(device);
  auto physical_device_it = device_map_.find(id);
  DCHECK(physical_device_it != device_map_.end());
  auto& device_infos = physical_device_it->second;
  auto device_it = base::ranges::find_if(
      device_infos, [&device](const device::mojom::HidDeviceInfoPtr& d) {
        return d->guid == device.guid;
      });
  DCHECK(device_it != device_infos.end());
  *device_it = device.Clone();
}

void HidChooserController::RunCallback(
    std::vector<device::mojom::HidDeviceInfoPtr> devices) {
  if (callback_) {
    std::move(callback_).Run(std::move(devices));
  }
}

void HidChooserController::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (hid_delegate_) {
    hid_delegate_->DeleteControllerForFrame(render_frame_host);
  }
}

}  // 命名空间电子
