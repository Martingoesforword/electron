// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/lib/bluetooth_chooser.h"

#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"

namespace gin {

template <>
struct Converter<electron::BluetoothChooser::DeviceInfo> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const electron::BluetoothChooser::DeviceInfo& val) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.Set("deviceName", val.device_name);
    dict.Set("deviceId", val.device_id);
    return gin::ConvertToV8(isolate, dict);
  }
};

}  // 命名空间杜松子酒。

namespace electron {

namespace {

const int kMaxScanRetries = 5;

void OnDeviceChosen(const content::BluetoothChooser::EventHandler& handler,
                    const std::string& device_id) {
  if (device_id.empty()) {
    handler.Run(content::BluetoothChooserEvent::CANCELLED, device_id);
  } else {
    handler.Run(content::BluetoothChooserEvent::SELECTED, device_id);
  }
}

}  // 命名空间。

BluetoothChooser::BluetoothChooser(api::WebContents* contents,
                                   const EventHandler& event_handler)
    : api_web_contents_(contents), event_handler_(event_handler) {}

BluetoothChooser::~BluetoothChooser() {
  event_handler_.Reset();
}

void BluetoothChooser::SetAdapterPresence(AdapterPresence presence) {
  switch (presence) {
    case AdapterPresence::ABSENT:
    case AdapterPresence::POWERED_OFF:
    // Chrome当前将用户定向到系统首选项。
    // 要授予这种情况下的蓝牙许可，我们是否应该。
    // 做类似的事吗？
    // Https://chromium-review.googlesource.com/c/chromium/src/+/2617129。
    case AdapterPresence::UNAUTHORIZED:
      event_handler_.Run(content::BluetoothChooserEvent::CANCELLED, "");
      break;
    case AdapterPresence::POWERED_ON:
      rescan_ = true;
      break;
  }
}

void BluetoothChooser::ShowDiscoveryState(DiscoveryState state) {
  switch (state) {
    case DiscoveryState::FAILED_TO_START:
      refreshing_ = false;
      event_handler_.Run(content::BluetoothChooserEvent::CANCELLED, "");
      break;
    case DiscoveryState::IDLE:
      refreshing_ = false;
      if (device_map_.empty()) {
        auto event = ++num_retries_ > kMaxScanRetries
                         ? content::BluetoothChooserEvent::CANCELLED
                         : content::BluetoothChooserEvent::RESCAN;
        event_handler_.Run(event, "");
      } else {
        bool prevent_default = api_web_contents_->Emit(
            "select-bluetooth-device", GetDeviceList(),
            base::BindOnce(&OnDeviceChosen, event_handler_));
        if (!prevent_default) {
          auto it = device_map_.begin();
          auto device_id = it->first;
          event_handler_.Run(content::BluetoothChooserEvent::SELECTED,
                             device_id);
        }
      }
      break;
    case DiscoveryState::DISCOVERING:
      // 第一次触发此状态是由于重新扫描触发，因此将。
      // 忽略设备的标志。
      if (rescan_ && !refreshing_) {
        refreshing_ = true;
      } else {
        // 第二次触发此状态时，我们现在可以安全地选择设备。
        refreshing_ = false;
      }
      break;
  }
}

void BluetoothChooser::AddOrUpdateDevice(const std::string& device_id,
                                         bool should_update_name,
                                         const std::u16string& device_name,
                                         bool is_gatt_connected,
                                         bool is_paired,
                                         int signal_strength_level) {
  if (refreshing_) {
    // 如果当前正在生成蓝牙设备列表，则不要触发。
    // 一件事。
    return;
  }
  bool changed = false;
  auto entry = device_map_.find(device_id);
  if (entry == device_map_.end()) {
    device_map_[device_id] = device_name;
    changed = true;
  } else if (should_update_name) {
    entry->second = device_name;
    changed = true;
  }

  if (changed) {
    // 发出选择蓝牙设备处理程序以允许用户监听。
    // 找到蓝牙设备。
    bool prevent_default = api_web_contents_->Emit(
        "select-bluetooth-device", GetDeviceList(),
        base::BindOnce(&OnDeviceChosen, event_handler_));

    // 如果未实现发射，请选择第一个与筛选器匹配的设备。
    // 如果是这样的话。
    if (!prevent_default) {
      event_handler_.Run(content::BluetoothChooserEvent::SELECTED, device_id);
    }
  }
}

std::vector<electron::BluetoothChooser::DeviceInfo>
BluetoothChooser::GetDeviceList() {
  std::vector<electron::BluetoothChooser::DeviceInfo> vec;
  vec.reserve(device_map_.size());
  for (const auto& it : device_map_) {
    DeviceInfo info = {it.first, it.second};
    vec.push_back(info);
  }

  return vec;
}

}  // 命名空间电子
