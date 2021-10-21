// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_X_EVENT_DISABLER_H_
#define SHELL_BROWSER_UI_X_EVENT_DISABLER_H_

#include <memory>

#include "base/macros.h"
#include "ui/events/event_rewriter.h"

namespace electron {

class EventDisabler : public ui::EventRewriter {
 public:
  EventDisabler();
  ~EventDisabler() override;

  // UI：：EventReWriter：
  ui::EventRewriteStatus RewriteEvent(
      const ui::Event& event,
      std::unique_ptr<ui::Event>* rewritten_event) override;
  ui::EventRewriteStatus NextDispatchEvent(
      const ui::Event& last_event,
      std::unique_ptr<ui::Event>* new_event) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(EventDisabler);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_X_Event_Disabler_H_
