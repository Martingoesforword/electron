// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_WIN_TASKBAR_HOST_H_
#define SHELL_BROWSER_UI_WIN_TASKBAR_HOST_H_

#include <shobjidl.h>
#include <wrl/client.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "shell/browser/native_window.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"

namespace electron {

class TaskbarHost {
 public:
  struct ThumbarButton {
    std::string tooltip;
    gfx::Image icon;
    std::vector<std::string> flags;
    base::RepeatingClosure clicked_callback;

    ThumbarButton();
    ThumbarButton(const ThumbarButton&);
    ~ThumbarButton();
  };

  TaskbarHost();
  virtual ~TaskbarHost();

  // 添加或更新拇指条中的按钮。
  bool SetThumbarButtons(HWND window,
                         const std::vector<ThumbarButton>& buttons);

  void RestoreThumbarButtons(HWND window);

  // 在任务栏中设置进度状态。
  bool SetProgressBar(HWND window,
                      double value,
                      const NativeWindow::ProgressState state);

  // 在任务栏中设置覆盖图标。
  bool SetOverlayIcon(HWND window,
                      const SkBitmap& overlay,
                      const std::string& text);

  // 将窗口区域设置为任务栏中的缩略图。
  bool SetThumbnailClip(HWND window, const gfx::Rect& region);

  // 在任务栏中设置缩略图的工具提示。
  bool SetThumbnailToolTip(HWND window, const std::string& tooltip);

  // 由点击了拇指条中的按钮的窗口调用。
  bool HandleThumbarButtonEvent(int button_id);

  void SetThumbarButtonsAdded(bool added) { thumbar_buttons_added_ = added; }

 private:
  // 初始化任务栏对象。
  bool InitializeTaskbar();

  using CallbackMap = std::map<int, base::RepeatingClosure>;
  CallbackMap callback_map_;

  std::vector<ThumbarButton> last_buttons_;

  // 任务栏的COM对象。
  Microsoft::WRL::ComPtr<ITaskbarList3> taskbar_;

  // 我们是否已经将按钮添加到拇指条。
  bool thumbar_buttons_added_ = false;

  DISALLOW_COPY_AND_ASSIGN(TaskbarHost);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Win_Taskbar_HOST_H_
