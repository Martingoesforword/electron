// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_GTK_MENU_UTIL_H_
#define SHELL_BROWSER_UI_GTK_MENU_UTIL_H_

#include <gtk/gtk.h>

#include <string>

#include "ui/gfx/image/image.h"

namespace ui {
class MenuModel;
}

namespace electron {

namespace gtkui {

// 生成GtkImageMenuItems。
GtkWidget* BuildMenuItemWithImage(const std::string& label, GtkWidget* image);
GtkWidget* BuildMenuItemWithImage(const std::string& label,
                                  const gfx::Image& icon);
GtkWidget* BuildMenuItemWithLabel(const std::string& label);

ui::MenuModel* ModelForMenuItem(GtkMenuItem* menu_item);

// 此方法用于动态构建菜单。返回值为。
// 新建菜单项。
GtkWidget* AppendMenuItemToMenu(int index,
                                ui::MenuModel* model,
                                GtkWidget* menu_item,
                                GtkWidget* menu,
                                bool connect_to_activate,
                                GCallback item_activated_cb,
                                void* this_ptr);

// 获取菜单项的ID。
// 如果菜单项有ID，则返回TRUE。
bool GetMenuItemID(GtkWidget* menu_item, int* menu_id);

// 执行与指定ID关联的命令。
void ExecuteCommand(ui::MenuModel* model, int id);

// 从|model_|创建GtkMenu。BLOCK_ACTIVATION_PTR用于禁用。
// 设置菜单项时的Item_Activate_Callback。
// 有关详细信息，请参阅SetMenuItemInfo定义中的注释。
void BuildSubmenuFromModel(ui::MenuModel* model,
                           GtkWidget* menu,
                           GCallback item_activated_cb,
                           bool* block_activation,
                           void* this_ptr);

// 设置菜单项上的复选标记、启用/禁用状态和动态标签。
void SetMenuItemInfo(GtkWidget* widget, void* block_activation_ptr);

}  // 命名空间gtkui。

}  // 命名空间电子。

#endif  // Shell_Browser_UI_GTK_MENU_UTIL_H_
