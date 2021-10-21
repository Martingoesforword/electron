// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_MENU_MAC_H_
#define SHELL_BROWSER_API_ELECTRON_API_MENU_MAC_H_

#include "shell/browser/api/electron_api_menu.h"

#include <map>

#import "shell/browser/ui/cocoa/electron_menu_controller.h"

using base::scoped_nsobject;

namespace electron {

namespace api {

class MenuMac : public Menu {
 protected:
  explicit MenuMac(gin::Arguments* args);
  ~MenuMac() override;

  void PopupAt(BaseWindow* window,
               int x,
               int y,
               int positioning_item,
               base::OnceClosure callback) override;
  void PopupOnUI(const base::WeakPtr<NativeWindow>& native_window,
                 int32_t window_id,
                 int x,
                 int y,
                 int positioning_item,
                 base::OnceClosure callback);
  void ClosePopupAt(int32_t window_id) override;
  std::u16string GetAcceleratorTextAtForTesting(int index) const override;

 private:
  friend class Menu;

  void ClosePopupOnUI(int32_t window_id);
  void OnClosed(int32_t window_id, base::OnceClosure callback);

  scoped_nsobject<ElectronMenuController> menu_controller_;

  // 窗口ID-&gt;打开上下文菜单。
  std::map<int32_t, scoped_nsobject<ElectronMenuController>> popup_controllers_;

  base::WeakPtrFactory<MenuMac> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(MenuMac);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Electronics_API_Menu_MAC_H_
