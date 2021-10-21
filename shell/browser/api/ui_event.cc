// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/ui_event.h"

#include "gin/data_object_builder.h"
#include "shell/browser/javascript_environment.h"
#include "ui/events/event_constants.h"
#include "v8/include/v8.h"

namespace electron {
namespace api {

constexpr int mouse_button_flags =
    (ui::EF_RIGHT_MOUSE_BUTTON | ui::EF_LEFT_MOUSE_BUTTON |
     ui::EF_MIDDLE_MOUSE_BUTTON | ui::EF_BACK_MOUSE_BUTTON |
     ui::EF_FORWARD_MOUSE_BUTTON);

v8::Local<v8::Object> CreateEventFromFlags(int flags) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  const int is_mouse_click = static_cast<bool>(flags & mouse_button_flags);
  return gin::DataObjectBuilder(isolate)
      .Set("shiftKey", static_cast<bool>(flags & ui::EF_SHIFT_DOWN))
      .Set("ctrlKey", static_cast<bool>(flags & ui::EF_CONTROL_DOWN))
      .Set("altKey", static_cast<bool>(flags & ui::EF_ALT_DOWN))
      .Set("metaKey", static_cast<bool>(flags & ui::EF_COMMAND_DOWN))
      .Set("triggeredByAccelerator", !is_mouse_click)
      .Build();
}

}  // 命名空间API。
}  // 命名空间电子
