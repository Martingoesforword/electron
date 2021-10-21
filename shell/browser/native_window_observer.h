// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NATIVE_WINDOW_OBSERVER_H_
#define SHELL_BROWSER_NATIVE_WINDOW_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"
#include "base/values.h"
#include "ui/base/window_open_disposition.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

class GURL;

namespace gfx {
class Rect;
enum class ResizeEdge;
}  // 命名空间gfx。

namespace electron {

class NativeWindowObserver : public base::CheckedObserver {
 public:
  ~NativeWindowObserver() override {}

  // 当Window中的网页想要创建弹出窗口时调用。
  virtual void WillCreatePopupWindow(const std::u16string& frame_name,
                                     const GURL& target_url,
                                     const std::string& partition_id,
                                     WindowOpenDisposition disposition) {}

  // 当用户在网页中启动导航时调用。
  virtual void WillNavigate(bool* prevent_default, const GURL& url) {}

  // 在窗户要关上的时候打来电话。
  virtual void WillCloseWindow(bool* prevent_default) {}

  // 当窗口想要知道首选宽度时调用。
  virtual void RequestPreferredWidth(int* width) {}

  // 在单击Closed按钮时调用。
  virtual void OnCloseButtonClicked(bool* prevent_default) {}

  // 在窗口关闭时调用。
  virtual void OnWindowClosed() {}

  // 当Windows发送WM_ENDSESSION消息时调用。
  virtual void OnWindowEndSession() {}

  // 当窗口失去焦点时调用。
  virtual void OnWindowBlur() {}

  // 当窗口获得焦点时调用。
  virtual void OnWindowFocus() {}

  // 当窗口获得或丢失关键窗口状态时调用。
  virtual void OnWindowIsKeyChanged(bool is_key) {}

  // 在显示窗口时调用。
  virtual void OnWindowShow() {}

  // 当窗口隐藏时调用。
  virtual void OnWindowHide() {}

  // 在窗口状态更改时调用。
  virtual void OnWindowMaximize() {}
  virtual void OnWindowUnmaximize() {}
  virtual void OnWindowMinimize() {}
  virtual void OnWindowRestore() {}
  virtual void OnWindowWillResize(const gfx::Rect& new_bounds,
                                  const gfx::ResizeEdge& edge,
                                  bool* prevent_default) {}
  virtual void OnWindowResize() {}
  virtual void OnWindowResized() {}
  virtual void OnWindowWillMove(const gfx::Rect& new_bounds,
                                bool* prevent_default) {}
  virtual void OnWindowMove() {}
  virtual void OnWindowMoved() {}
  virtual void OnWindowScrollTouchBegin() {}
  virtual void OnWindowScrollTouchEnd() {}
  virtual void OnWindowSwipe(const std::string& direction) {}
  virtual void OnWindowRotateGesture(float rotation) {}
  virtual void OnWindowSheetBegin() {}
  virtual void OnWindowSheetEnd() {}
  virtual void OnWindowEnterFullScreen() {}
  virtual void OnWindowLeaveFullScreen() {}
  virtual void OnWindowEnterHtmlFullScreen() {}
  virtual void OnWindowLeaveHtmlFullScreen() {}
  virtual void OnWindowAlwaysOnTopChanged() {}
  virtual void OnTouchBarItemResult(const std::string& item_id,
                                    const base::DictionaryValue& details) {}
  virtual void OnNewWindowForTab() {}
  virtual void OnSystemContextMenu(int x, int y, bool* prevent_default) {}

// 在收到窗口消息时调用。
#if defined(OS_WIN)
  virtual void OnWindowMessage(UINT message, WPARAM w_param, LPARAM l_param) {}
#endif

  // 应用程序命令到达时在Windows上调用(WM_APPCOMMAND)。
  // 有些命令也在其他平台上实现。
  virtual void OnExecuteAppCommand(const std::string& command_name) {}

  virtual void UpdateWindowControlsOverlay(const gfx::Rect& bounding_rect) {}
};

}  // 命名空间电子。

#endif  // Shell_Browser_Native_Window_Observator_H_
