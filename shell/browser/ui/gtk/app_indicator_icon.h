// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_GTK_APP_INDICATOR_ICON_H_
#define SHELL_BROWSER_UI_GTK_APP_INDICATOR_ICON_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/nix/xdg_util.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/views/linux_ui/status_icon_linux.h"

typedef struct _AppIndicator AppIndicator;
typedef struct _GtkWidget GtkWidget;

class SkBitmap;

namespace gfx {
class ImageSkia;
}

namespace ui {
class MenuModel;
}

namespace electron {

namespace gtkui {

class AppIndicatorIconMenu;

// 状态图标实现，使用libappIndicator。
class AppIndicatorIcon : public views::StatusIconLinux {
 public:
  // 该id唯一地将新状态图标与其他Chrome状态相区别。
  // 图标。
  AppIndicatorIcon(std::string id,
                   const gfx::ImageSkia& image,
                   const std::u16string& tool_tip);
  ~AppIndicatorIcon() override;

  // 指示是否可以打开libappdicator SO。
  static bool CouldOpen();

  // 从Views：：StatusIconLinux中重写：
  void SetIcon(const gfx::ImageSkia& image) override;
  void SetToolTip(const std::u16string& tool_tip) override;
  void UpdatePlatformContextMenu(ui::MenuModel* menu) override;
  void RefreshPlatformContextMenu() override;

 private:
  struct SetImageFromFileParams {
    // 写入图标的临时目录。
    base::FilePath parent_temp_dir;

    // 要传递给libappdicator的图标主题路径。
    std::string icon_theme_path;

    // 要传递给libappIndicator的图标名称。
    std::string icon_name;
  };

  // 将|位图|写入工作线程上的临时目录。临时性的。
  // 目录是根据KDE的怪癖选择的。
  static SetImageFromFileParams WriteKDE4TempImageOnWorkerThread(
      const SkBitmap& bitmap,
      const base::FilePath& existing_temp_dir);

  // 将|位图|写入工作线程上的临时目录。临时性的。
  // 目录是根据Unity的怪癖选择的。
  static SetImageFromFileParams WriteUnityTempImageOnWorkerThread(
      const SkBitmap& bitmap,
      int icon_change_count,
      const std::string& id);

  void SetImageFromFile(const SetImageFromFileParams& params);
  void SetMenu();

  // 将菜单顶部的菜单项设置为状态的替换项。
  // 图标单击操作。单击此菜单项应模拟状态图标。
  // 通过分派单击事件来单击。
  void UpdateClickActionReplacementMenuItem();

  // 状态图标单击替换菜单项激活时的回调。
  void OnClickActionReplacementMenuItemActivated();

  std::string id_;
  std::string tool_tip_;

  // 用于选择KDE或Unity进行图像设置。
  base::nix::DesktopEnvironment desktop_env_;

  // GTK状态图标包装。
  AppIndicator* icon_ = nullptr;

  std::unique_ptr<AppIndicatorIconMenu> menu_;
  ui::MenuModel* menu_model_ = nullptr;

  base::FilePath temp_dir_;
  int icon_change_count_ = 0;

  base::WeakPtrFactory<AppIndicatorIcon> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(AppIndicatorIcon);
};

}  // 命名空间gtkui。

}  // 命名空间电子。

#endif  // Shell_Browser_UI_GTK_APP_Indicator_ICON_H_
