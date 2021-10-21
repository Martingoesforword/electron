// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_GTK_GTK_STATUS_ICON_H_
#define SHELL_BROWSER_UI_GTK_GTK_STATUS_ICON_H_

#include <memory>

#include "base/macros.h"
#include "ui/base/glib/glib_integers.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/views/linux_ui/status_icon_linux.h"

typedef struct _GtkStatusIcon GtkStatusIcon;

namespace gfx {
class ImageSkia;
}

namespace ui {
class MenuModel;
}

namespace electron {

namespace gtkui {
class AppIndicatorIconMenu;

// 使用系统托盘X11规范(通过)的状态图标实现。
// GtkStatusIcon)。
class GtkStatusIcon : public views::StatusIconLinux {
 public:
  GtkStatusIcon(const gfx::ImageSkia& image, const std::u16string& tool_tip);
  ~GtkStatusIcon() override;

  // 从Views：：StatusIconLinux中重写：
  void SetIcon(const gfx::ImageSkia& image) override;
  void SetToolTip(const std::u16string& tool_tip) override;
  void UpdatePlatformContextMenu(ui::MenuModel* menu) override;
  void RefreshPlatformContextMenu() override;

 private:
  CHROMEG_CALLBACK_0(GtkStatusIcon, void, OnClick, GtkStatusIcon*);

  CHROMEG_CALLBACK_2(GtkStatusIcon,
                     void,
                     OnContextMenuRequested,
                     GtkStatusIcon*,
                     guint,
                     guint);

  ::GtkStatusIcon* gtk_status_icon_;

  std::unique_ptr<AppIndicatorIconMenu> menu_;

  DISALLOW_COPY_AND_ASSIGN(GtkStatusIcon);
};

}  // 命名空间gtkui。

}  // 命名空间电子。

#endif  // Shell_Browser_UI_GTK_GTK_STATUS_ICON_H_
