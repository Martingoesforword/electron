// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_SCREEN_H_
#define SHELL_BROWSER_API_ELECTRON_API_SCREEN_H_

#include <vector>

#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "ui/display/display_observer.h"
#include "ui/display/screen.h"

namespace gfx {
class Point;
class Rect;
class Screen;
}  // 命名空间gfx。

namespace electron {

namespace api {

class Screen : public gin::Wrappable<Screen>,
               public gin_helper::EventEmitterMixin<Screen>,
               public display::DisplayObserver {
 public:
  static v8::Local<v8::Value> Create(gin_helper::ErrorThrower error_thrower);

  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  Screen(v8::Isolate* isolate, display::Screen* screen);
  ~Screen() override;

  gfx::Point GetCursorScreenPoint();
  display::Display GetPrimaryDisplay();
  std::vector<display::Display> GetAllDisplays();
  display::Display GetDisplayNearestPoint(const gfx::Point& point);
  display::Display GetDisplayMatching(const gfx::Rect& match_rect);

  // Display：：DisplayViewer：
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

 private:
  display::Screen* screen_;

  DISALLOW_COPY_AND_ASSIGN(Screen);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Electronics_API_Screen_H_
