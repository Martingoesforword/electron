// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_TRAY_ICON_OBSERVER_H_
#define SHELL_BROWSER_UI_TRAY_ICON_OBSERVER_H_

#include <string>
#include <vector>

#include "base/observer_list_types.h"

namespace gfx {
class Rect;
class Point;
}  // 命名空间gfx。

namespace electron {

class TrayIconObserver : public base::CheckedObserver {
 public:
  virtual void OnClicked(const gfx::Rect& bounds,
                         const gfx::Point& location,
                         int modifiers) {}
  virtual void OnDoubleClicked(const gfx::Rect& bounds, int modifiers) {}
  virtual void OnBalloonShow() {}
  virtual void OnBalloonClicked() {}
  virtual void OnBalloonClosed() {}
  virtual void OnRightClicked(const gfx::Rect& bounds, int modifiers) {}
  virtual void OnDrop() {}
  virtual void OnDropFiles(const std::vector<std::string>& files) {}
  virtual void OnDropText(const std::string& text) {}
  virtual void OnDragEntered() {}
  virtual void OnDragExited() {}
  virtual void OnDragEnded() {}
  virtual void OnMouseUp(const gfx::Point& location, int modifiers) {}
  virtual void OnMouseDown(const gfx::Point& location, int modifiers) {}
  virtual void OnMouseEntered(const gfx::Point& location, int modifiers) {}
  virtual void OnMouseExited(const gfx::Point& location, int modifiers) {}
  virtual void OnMouseMoved(const gfx::Point& location, int modifiers) {}

 protected:
  ~TrayIconObserver() override {}
};

}  // 命名空间电子。

#endif  // 外壳浏览器UI托盘图标观察者H_
