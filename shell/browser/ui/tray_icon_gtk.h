// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_TRAY_ICON_GTK_H_
#define SHELL_BROWSER_UI_TRAY_ICON_GTK_H_

#include <memory>
#include <string>

#include "shell/browser/ui/tray_icon.h"
#include "ui/views/linux_ui/status_icon_linux.h"

namespace electron {

class TrayIconGtk : public TrayIcon, public views::StatusIconLinux::Delegate {
 public:
  TrayIconGtk();
  ~TrayIconGtk() override;

  // 托盘图标：
  void SetImage(const gfx::Image& image) override;
  void SetToolTip(const std::string& tool_tip) override;
  void SetContextMenu(ElectronMenuModel* menu_model) override;

  // 视图：：StatusIconLinux：：Delegate。
  void OnClick() override;
  bool HasClickAction() override;
  // 以下四种方法仅由StatusIconLinuxDbus使用，我们。
  // 还没有使用，所以会给他们提供存根实现。
  const gfx::ImageSkia& GetImage() const override;
  const std::u16string& GetToolTip() const override;
  ui::MenuModel* GetMenuModel() const override;
  void OnImplInitializationFailed() override;

 private:
  std::unique_ptr<views::StatusIconLinux> icon_;
  gfx::ImageSkia image_;
  std::u16string tool_tip_;
  ui::MenuModel* menu_model_;

  DISALLOW_COPY_AND_ASSIGN(TrayIconGtk);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Tray_ICON_GTK_H_
