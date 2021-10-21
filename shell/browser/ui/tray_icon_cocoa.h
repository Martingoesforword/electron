// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_TRAY_ICON_COCOA_H_
#define SHELL_BROWSER_UI_TRAY_ICON_COCOA_H_

#import <Cocoa/Cocoa.h>

#include <string>

#include "base/mac/scoped_nsobject.h"
#include "shell/browser/ui/tray_icon.h"

@class ElectronMenuController;
@class StatusItemView;

namespace electron {

class TrayIconCocoa : public TrayIcon {
 public:
  TrayIconCocoa();
  ~TrayIconCocoa() override;

  void SetImage(const gfx::Image& image) override;
  void SetPressedImage(const gfx::Image& image) override;
  void SetToolTip(const std::string& tool_tip) override;
  void SetTitle(const std::string& title, const TitleOptions& options) override;
  std::string GetTitle() override;
  void SetIgnoreDoubleClickEvents(bool ignore) override;
  bool GetIgnoreDoubleClickEvents() override;
  void PopUpOnUI(ElectronMenuModel* menu_model);
  void PopUpContextMenu(const gfx::Point& pos,
                        ElectronMenuModel* menu_model) override;
  void CloseContextMenu() override;
  void SetContextMenu(ElectronMenuModel* menu_model) override;
  gfx::Rect GetBounds() override;

 private:
  // NSStatusItem的电子自定义视图。
  base::scoped_nsobject<StatusItemView> status_item_view_;

  // 右键单击系统图标时显示的状态菜单。
  base::scoped_nsobject<ElectronMenuController> menu_;

  base::WeakPtrFactory<TrayIconCocoa> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(TrayIconCocoa);
};

}  // 命名空间电子。

#endif  // 外壳浏览器UI托盘图标可可H_
