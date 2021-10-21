// 版权所有2016年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_NAVIGATION_UI_DATA_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_NAVIGATION_UI_DATA_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/navigation_ui_data.h"
#include "extensions/browser/extension_navigation_ui_data.h"

namespace extensions {

// PlzNavigate。
// 包含从UI线程传递到位于。
// 每次导航的开始。类在UI线程上实例化，
// 然后，使用克隆创建的副本将传递给Content：：ResourceRequestInfo。
// 在IO线程上。
class ElectronNavigationUIData : public content::NavigationUIData {
 public:
  ElectronNavigationUIData();
  explicit ElectronNavigationUIData(
      content::NavigationHandle* navigation_handle);
  ~ElectronNavigationUIData() override;

  // 创建一个新的ChromeNavigationUIData，它是原始文件的深层副本。
  // 创建克隆后对原始文件所做的任何更改都不会。
  // 反映在克隆人身上。|EXTENSION_DATA_|被深度复制。
  std::unique_ptr<content::NavigationUIData> Clone() override;

  void SetExtensionNavigationUIData(
      std::unique_ptr<ExtensionNavigationUIData> extension_data);

  ExtensionNavigationUIData* GetExtensionNavigationUIData() const {
    return extension_data_.get();
  }

 private:
  // 管理可选ExtensionNavigationUIData信息的生存期。
  std::unique_ptr<ExtensionNavigationUIData> extension_data_;

  DISALLOW_COPY_AND_ASSIGN(ElectronNavigationUIData);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_NAVIGATION_UI_DATA_H_
