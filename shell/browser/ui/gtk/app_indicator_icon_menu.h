// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_GTK_APP_INDICATOR_ICON_MENU_H_
#define SHELL_BROWSER_UI_GTK_APP_INDICATOR_ICON_MENU_H_

#include "base/callback.h"
#include "base/macros.h"
#include "ui/base/glib/glib_signal.h"

typedef struct _GtkMenu GtkMenu;
typedef struct _GtkWidget GtkWidget;

namespace ui {
class MenuModel;
}

namespace electron {

namespace gtkui {

// 应用程序指示器图标的菜单。
class AppIndicatorIconMenu {
 public:
  explicit AppIndicatorIconMenu(ui::MenuModel* model);
  virtual ~AppIndicatorIconMenu();

  // 将|GTK_MENU_|顶部的菜单项设置为应用程序的替代项。
  // 指示器图标的单击操作。|callback|菜单项出现时调用。
  // 被激活。
  void UpdateClickActionReplacementMenuItem(
      const char* label,
      const base::RepeatingClosure& callback);

  // 刷新所有菜单项标签和菜单项选中/启用状态。
  void Refresh();

  GtkMenu* GetGtkMenu();

 private:
  // “点击操作替换”菜单项激活时的回调。
  CHROMEG_CALLBACK_0(AppIndicatorIconMenu,
                     void,
                     OnClickActionReplacementMenuItemActivated,
                     GtkWidget*);

  // 菜单项激活时的回调。
  CHROMEG_CALLBACK_0(AppIndicatorIconMenu,
                     void,
                     OnMenuItemActivated,
                     GtkWidget*);

  // 不是所有的。
  ui::MenuModel* menu_model_;

  // 菜单中是否添加了“单击操作替换”菜单项。
  bool click_action_replacement_menu_item_added_ = false;

  // 在激活单击操作替换菜单项时调用。当一个。
  // |MENU_MODEL_|中的菜单项已激活，MenuModel：：ActivatedAt()为。
  // 被调用，并被假定执行任何必要的处理。
  base::RepeatingClosure click_action_replacement_callback_;

  GtkWidget* gtk_menu_ = nullptr;

  bool block_activation_ = false;

  DISALLOW_COPY_AND_ASSIGN(AppIndicatorIconMenu);
};

}  // 命名空间gtkui。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_GTK_APP_INDICATOR_ICON_MENU_H_
