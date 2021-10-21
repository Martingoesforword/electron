// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_
#define SHELL_BROWSER_LIB_BLUETOOTH_CHOOSER_H_

#include <map>
#include <string>
#include <vector>

#include "content/public/browser/bluetooth_chooser.h"
#include "shell/browser/api/electron_api_web_contents.h"

namespace electron {

class BluetoothChooser : public content::BluetoothChooser {
 public:
  struct DeviceInfo {
    std::string device_id;
    std::u16string device_name;
  };

  explicit BluetoothChooser(api::WebContents* contents,
                            const EventHandler& handler);
  ~BluetoothChooser() override;

  // 内容：：蓝牙选择器：
  void SetAdapterPresence(AdapterPresence presence) override;
  void ShowDiscoveryState(DiscoveryState state) override;
  void AddOrUpdateDevice(const std::string& device_id,
                         bool should_update_name,
                         const std::u16string& device_name,
                         bool is_gatt_connected,
                         bool is_paired,
                         int signal_strength_level) override;
  std::vector<DeviceInfo> GetDeviceList();

 private:
  std::map<std::string, std::u16string> device_map_;
  api::WebContents* api_web_contents_;
  EventHandler event_handler_;
  int num_retries_ = 0;
  bool refreshing_ = false;
  bool rescan_ = false;

  DISALLOW_COPY_AND_ASSIGN(BluetoothChooser);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Lib_Bluetooth_Chooser_H_
