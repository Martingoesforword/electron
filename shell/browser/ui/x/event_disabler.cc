// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/x/event_disabler.h"

#include <memory>

namespace electron {

EventDisabler::EventDisabler() = default;

EventDisabler::~EventDisabler() = default;

ui::EventRewriteStatus EventDisabler::RewriteEvent(
    const ui::Event& event,
    std::unique_ptr<ui::Event>* rewritten_event) {
  return ui::EVENT_REWRITE_DISCARD;
}

ui::EventRewriteStatus EventDisabler::NextDispatchEvent(
    const ui::Event& last_event,
    std::unique_ptr<ui::Event>* new_event) {
  return ui::EVENT_REWRITE_CONTINUE;
}

}  // 命名空间电子
