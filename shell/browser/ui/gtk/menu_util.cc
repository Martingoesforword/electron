// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/gtk/menu_util.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <map>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "shell/browser/ui/gtk_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkUnPreMultiply.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/menu_label_accelerator_util_linux.h"
#include "ui/base/models/image_model.h"
#include "ui/base/models/menu_model.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_code_conversion_x.h"

#if defined(USE_OZONE) || defined(USE_X11)
#include "ui/base/ui_base_features.h"
#endif

namespace electron {

namespace gtkui {

namespace {

int EventFlagsFromGdkState(guint state) {
  int flags = ui::EF_NONE;
  flags |= (state & GDK_SHIFT_MASK) ? ui::EF_SHIFT_DOWN : ui::EF_NONE;
  flags |= (state & GDK_LOCK_MASK) ? ui::EF_CAPS_LOCK_ON : ui::EF_NONE;
  flags |= (state & GDK_CONTROL_MASK) ? ui::EF_CONTROL_DOWN : ui::EF_NONE;
  flags |= (state & GDK_MOD1_MASK) ? ui::EF_ALT_DOWN : ui::EF_NONE;
  flags |= (state & GDK_BUTTON1_MASK) ? ui::EF_LEFT_MOUSE_BUTTON : ui::EF_NONE;
  flags |=
      (state & GDK_BUTTON2_MASK) ? ui::EF_MIDDLE_MOUSE_BUTTON : ui::EF_NONE;
  flags |= (state & GDK_BUTTON3_MASK) ? ui::EF_RIGHT_MOUSE_BUTTON : ui::EF_NONE;
  return flags;
}

guint GetGdkKeyCodeForAccelerator(const ui::Accelerator& accelerator) {
  // 第二个参数为FALSE，因为快捷键表示为。
  // 非Shift-Modified键的术语。
  return XKeysymForWindowsKeyCode(accelerator.key_code(), false);
}

GdkModifierType GetGdkModifierForAccelerator(
    const ui::Accelerator& accelerator) {
  int event_flag = accelerator.modifiers();
  int modifier = 0;
  if (event_flag & ui::EF_SHIFT_DOWN)
    modifier |= GDK_SHIFT_MASK;
  if (event_flag & ui::EF_CONTROL_DOWN)
    modifier |= GDK_CONTROL_MASK;
  if (event_flag & ui::EF_ALT_DOWN)
    modifier |= GDK_MOD1_MASK;
  return static_cast<GdkModifierType>(modifier);
}

}  // 命名空间。

GtkWidget* BuildMenuItemWithImage(const std::string& label, GtkWidget* image) {
// GTK4删除了对图像菜单项的支持。
#if GTK_CHECK_VERSION(3, 90, 0)
  return gtk_menu_item_new_with_mnemonic(label.c_str());
#else
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
  GtkWidget* menu_item = gtk_image_menu_item_new_with_mnemonic(label.c_str());
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image);
  G_GNUC_END_IGNORE_DEPRECATIONS;
  return menu_item;
#endif
}

GtkWidget* BuildMenuItemWithImage(const std::string& label,
                                  const gfx::Image& icon) {
  GdkPixbuf* pixbuf = gtk_util::GdkPixbufFromSkBitmap(*icon.ToSkBitmap());

  GtkWidget* menu_item =
      BuildMenuItemWithImage(label, gtk_image_new_from_pixbuf(pixbuf));
  g_object_unref(pixbuf);
  return menu_item;
}

GtkWidget* BuildMenuItemWithLabel(const std::string& label) {
  return gtk_menu_item_new_with_mnemonic(label.c_str());
}

ui::MenuModel* ModelForMenuItem(GtkMenuItem* menu_item) {
  return reinterpret_cast<ui::MenuModel*>(
      g_object_get_data(G_OBJECT(menu_item), "model"));
}

GtkWidget* AppendMenuItemToMenu(int index,
                                ui::MenuModel* model,
                                GtkWidget* menu_item,
                                GtkWidget* menu,
                                bool connect_to_activate,
                                GCallback item_activated_cb,
                                void* this_ptr) {
  // 设置菜单项的ID。
  // 将Menu_id加1，以避免将零(NULL)设置为“Menu-id”。
  g_object_set_data(G_OBJECT(menu_item), "menu-id", GINT_TO_POINTER(index + 1));

  // 本机菜单项做它们自己的事情，所以只有选择地监听。
  // 激活信号。
  if (connect_to_activate) {
    g_signal_connect(menu_item, "activate", item_activated_cb, this_ptr);
  }

  // 当我们控制菜单创建时，AppendMenuItemToMenu在内部使用。
  // 从模型(其中模型可以选择隐藏某些菜单项)，以及。
  // 使用不提供选项的即时命令。
  if (model) {
    if (model->IsVisibleAt(index))
      gtk_widget_show(menu_item);
  } else {
    gtk_widget_show(menu_item);
  }
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
  return menu_item;
}

bool GetMenuItemID(GtkWidget* menu_item, int* menu_id) {
  gpointer id_ptr = g_object_get_data(G_OBJECT(menu_item), "menu-id");
  if (id_ptr != nullptr) {
    *menu_id = GPOINTER_TO_INT(id_ptr) - 1;
    return true;
  }

  return false;
}

void ExecuteCommand(ui::MenuModel* model, int id) {
  GdkEvent* event = gtk_get_current_event();
  int event_flags = 0;

  if (event && event->type == GDK_BUTTON_RELEASE)
    event_flags = EventFlagsFromGdkState(event->button.state);
  model->ActivatedAt(id, event_flags);

  if (event)
    gdk_event_free(event);
}

void BuildSubmenuFromModel(ui::MenuModel* model,
                           GtkWidget* menu,
                           GCallback item_activated_cb,
                           bool* block_activation,
                           void* this_ptr) {
  std::map<int, GtkWidget*> radio_groups;
  GtkWidget* menu_item = nullptr;
  for (int i = 0; i < model->GetItemCount(); ++i) {
    std::string label = ui::ConvertAcceleratorsFromWindowsStyle(
        base::UTF16ToUTF8(model->GetLabelAt(i)));

    bool connect_to_activate = true;

    switch (model->GetTypeAt(i)) {
      case ui::MenuModel::TYPE_SEPARATOR:
        menu_item = gtk_separator_menu_item_new();
        break;

      case ui::MenuModel::TYPE_CHECK:
        menu_item = gtk_check_menu_item_new_with_mnemonic(label.c_str());
        break;

      case ui::MenuModel::TYPE_RADIO: {
        auto iter = radio_groups.find(model->GetGroupIdAt(i));

        if (iter == radio_groups.end()) {
          menu_item =
              gtk_radio_menu_item_new_with_mnemonic(nullptr, label.c_str());
          radio_groups[model->GetGroupIdAt(i)] = menu_item;
        } else {
          menu_item = gtk_radio_menu_item_new_with_mnemonic_from_widget(
              GTK_RADIO_MENU_ITEM(iter->second), label.c_str());
        }
        break;
      }
      case ui::MenuModel::TYPE_BUTTON_ITEM: {
        NOTIMPLEMENTED();
        break;
      }
      case ui::MenuModel::TYPE_SUBMENU:
      case ui::MenuModel::TYPE_COMMAND: {
        auto icon_model = model->GetIconAt(i);
        if (!icon_model.IsEmpty())
          menu_item = BuildMenuItemWithImage(label, icon_model.GetImage());
        else
          menu_item = BuildMenuItemWithLabel(label);
#if !GTK_CHECK_VERSION(3, 90, 0)
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        if (GTK_IS_IMAGE_MENU_ITEM(menu_item)) {
          gtk_image_menu_item_set_always_show_image(
              GTK_IMAGE_MENU_ITEM(menu_item), TRUE);
        }
        G_GNUC_END_IGNORE_DEPRECATIONS;
#endif
        break;
      }

      default:
        NOTREACHED();
    }

    if (model->GetTypeAt(i) == ui::MenuModel::TYPE_SUBMENU) {
      GtkWidget* submenu = gtk_menu_new();
      ui::MenuModel* submenu_model = model->GetSubmenuModelAt(i);
      BuildSubmenuFromModel(submenu_model, submenu, item_activated_cb,
                            block_activation, this_ptr);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);

      // 更新新生成的菜单中的所有菜单项信息。
      gtk_container_foreach(GTK_CONTAINER(submenu), SetMenuItemInfo,
                            block_activation);
      submenu_model->MenuWillShow();
      connect_to_activate = false;
    }

#if defined(USE_X11)
    ui::Accelerator accelerator;
    if (model->GetAcceleratorAt(i, &accelerator)) {
      gtk_widget_add_accelerator(menu_item, "activate", nullptr,
                                 GetGdkKeyCodeForAccelerator(accelerator),
                                 GetGdkModifierForAccelerator(accelerator),
                                 GTK_ACCEL_VISIBLE);
    }
#endif

    g_object_set_data(G_OBJECT(menu_item), "model", model);
    AppendMenuItemToMenu(i, model, menu_item, menu, connect_to_activate,
                         item_activated_cb, this_ptr);

    menu_item = nullptr;
  }
}

void SetMenuItemInfo(GtkWidget* widget, void* block_activation_ptr) {
  if (GTK_IS_SEPARATOR_MENU_ITEM(widget)) {
    // 我们需要显式处理此情况，否则我们将要求。
    // 有关ID无效的内容的菜单委托。
    return;
  }

  int id;
  if (!GetMenuItemID(widget, &id))
    return;

  ui::MenuModel* model = ModelForMenuItem(GTK_MENU_ITEM(widget));
  if (!model) {
    // 如果我们不提供子菜单，那么就没有型号。为。
    // 例如，IME子菜单没有型号。
    return;
  }
  bool* block_activation = static_cast<bool*>(block_activation_ptr);

  if (GTK_IS_CHECK_MENU_ITEM(widget)) {
    GtkCheckMenuItem* item = GTK_CHECK_MENU_ITEM(widget);

    // GTK_CHECK_MENU_ITEM_SET_ACTIVE()将发送激活信号。感人至深。
    // 基础的“active”属性还将调用“activate”处理程序。
    // 此菜单项。因此，我们防止“激活”处理程序。
    // 在我们设置复选框时被调用。
    // 为什么不使用Glib信号阻塞函数之一呢？因为当我们。
    // 切换单选按钮，它将停用其他单选按钮之一，
    // 我们没有指向它的指针。
    *block_activation = true;
    gtk_check_menu_item_set_active(item, model->IsItemCheckedAt(id));
    *block_activation = false;
  }

  if (GTK_IS_MENU_ITEM(widget)) {
    gtk_widget_set_sensitive(widget, model->IsEnabledAt(id));

    if (model->IsVisibleAt(id)) {
      // 如果菜单项标签是动态的，请更新该菜单项标签。
      if (model->IsItemDynamicAt(id)) {
        std::string label = ui::ConvertAcceleratorsFromWindowsStyle(
            base::UTF16ToUTF8(model->GetLabelAt(id)));

        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), label.c_str());
#if !GTK_CHECK_VERSION(3, 90, 0)
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
        if (GTK_IS_IMAGE_MENU_ITEM(widget)) {
          auto icon_model = model->GetIconAt(id);
          if (!icon_model.IsEmpty()) {
            GdkPixbuf* pixbuf = gtk_util::GdkPixbufFromSkBitmap(
                *icon_model.GetImage().ToSkBitmap());
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(widget),
                                          gtk_image_new_from_pixbuf(pixbuf));
            g_object_unref(pixbuf);
          } else {
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(widget), nullptr);
          }
        }
        G_GNUC_END_IGNORE_DEPRECATIONS;
#endif
      }

      gtk_widget_show(widget);
    } else {
      gtk_widget_hide(widget);
    }

    GtkWidget* submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));
    if (submenu) {
      gtk_container_foreach(GTK_CONTAINER(submenu), &SetMenuItemInfo,
                            block_activation_ptr);
    }
  }
}

}  // 命名空间gtkui。

}  // 命名空间电子
