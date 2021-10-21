// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_TRAY_ICON_H_
#define SHELL_BROWSER_UI_TRAY_ICON_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/browser/ui/tray_icon_observer.h"
#include "shell/common/gin_converters/guid_converter.h"
#include "ui/gfx/geometry/rect.h"

namespace electron {

class TrayIcon {
 public:
  static TrayIcon* Create(absl::optional<UUID> guid);

#if defined(OS_WIN)
  using ImageType = HICON;
#else
  using ImageType = const gfx::Image&;
#endif

  virtual ~TrayIcon();

  // 设置与此状态图标关联的图像。
  virtual void SetImage(ImageType image) = 0;

  // 设置按下时与此状态图标关联的图像。
  virtual void SetPressedImage(ImageType image);

  // 设置此状态图标的悬停文本。这也用作标签。
  // 对于作为状态图标的替换项创建的菜单项。
  // 在不支持自定义单击操作的平台上执行单击操作。
  // 状态图标(例如Ubuntu Unity)。
  virtual void SetToolTip(const std::string& tool_tip) = 0;

#if defined(OS_MAC)
  // 设置/获取标志，确定是否忽略双击事件。
  virtual void SetIgnoreDoubleClickEvents(bool ignore) = 0;
  virtual bool GetIgnoreDoubleClickEvents() = 0;

  struct TitleOptions {
    std::string font_type;
  };

  // 设置/获取标题显示在状态栏中状态图标旁边。
  virtual void SetTitle(const std::string& title,
                        const TitleOptions& options) = 0;
  virtual std::string GetTitle() = 0;
#endif

  enum class IconType { kNone, kInfo, kWarning, kError, kCustom };

  struct BalloonOptions {
    IconType icon_type = IconType::kCustom;
#if defined(OS_WIN)
    HICON icon = nullptr;
#else
    gfx::Image icon;
#endif
    std::u16string title;
    std::u16string content;
    bool large_icon = true;
    bool no_sound = false;
    bool respect_quiet_time = false;

    BalloonOptions();
  };

  // 显示具有指定内容的通知气球。
  // 根据平台的不同，它可能不会出现在图标托盘上。
  virtual void DisplayBalloon(const BalloonOptions& options);

  // 删除通知气球。
  virtual void RemoveBalloon();

  // 将焦点返回到任务栏通知区域。
  virtual void Focus();

  // 弹出菜单。
  virtual void PopUpContextMenu(const gfx::Point& pos,
                                ElectronMenuModel* menu_model);

  virtual void CloseContextMenu();

  // 设置此图标的上下文菜单。
  virtual void SetContextMenu(ElectronMenuModel* menu_model) = 0;

  // 返回托盘图标的边界。
  virtual gfx::Rect GetBounds();

  void AddObserver(TrayIconObserver* obs) { observers_.AddObserver(obs); }
  void RemoveObserver(TrayIconObserver* obs) { observers_.RemoveObserver(obs); }

  void NotifyClicked(const gfx::Rect& = gfx::Rect(),
                     const gfx::Point& location = gfx::Point(),
                     int modifiers = 0);
  void NotifyDoubleClicked(const gfx::Rect& = gfx::Rect(), int modifiers = 0);
  void NotifyBalloonShow();
  void NotifyBalloonClicked();
  void NotifyBalloonClosed();
  void NotifyRightClicked(const gfx::Rect& bounds = gfx::Rect(),
                          int modifiers = 0);
  void NotifyDrop();
  void NotifyDropFiles(const std::vector<std::string>& files);
  void NotifyDropText(const std::string& text);
  void NotifyDragEntered();
  void NotifyDragExited();
  void NotifyDragEnded();
  void NotifyMouseUp(const gfx::Point& location = gfx::Point(),
                     int modifiers = 0);
  void NotifyMouseDown(const gfx::Point& location = gfx::Point(),
                       int modifiers = 0);
  void NotifyMouseEntered(const gfx::Point& location = gfx::Point(),
                          int modifiers = 0);
  void NotifyMouseExited(const gfx::Point& location = gfx::Point(),
                         int modifiers = 0);
  void NotifyMouseMoved(const gfx::Point& location = gfx::Point(),
                        int modifiers = 0);

 protected:
  TrayIcon();

 private:
  base::ObserverList<TrayIconObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(TrayIcon);
};

}  // 命名空间电子。

#endif  // 外壳浏览器UI托盘图标H_
