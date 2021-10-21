// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/views/menu_delegate.h"

#include <memory>

#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/ui/views/menu_bar.h"
#include "shell/browser/ui/views/menu_model_adapter.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/widget/widget.h"

namespace electron {

MenuDelegate::MenuDelegate(MenuBar* menu_bar) : menu_bar_(menu_bar) {}

MenuDelegate::~MenuDelegate() = default;

void MenuDelegate::RunMenu(ElectronMenuModel* model,
                           views::Button* button,
                           ui::MenuSourceType source_type) {
  gfx::Point screen_loc;
  views::View::ConvertPointToScreen(button, &screen_loc);
  // 从高度减去1，使弹出窗口与按钮边框齐平。
  gfx::Rect bounds(screen_loc.x(), screen_loc.y(), button->width(),
                   button->height() - 1);

  if (source_type == ui::MENU_SOURCE_KEYBOARD) {
    hold_first_switch_ = true;
  }

  id_ = button->tag();
  adapter_ = std::make_unique<MenuModelAdapter>(model);

  auto* item = new views::MenuItemView(this);
  static_cast<MenuModelAdapter*>(adapter_.get())->BuildMenu(item);

  menu_runner_ = std::make_unique<views::MenuRunner>(
      item, views::MenuRunner::CONTEXT_MENU | views::MenuRunner::HAS_MNEMONICS);
  menu_runner_->RunMenuAt(
      button->GetWidget()->GetTopLevelWidget(),
      static_cast<views::MenuButton*>(button)->button_controller(), bounds,
      views::MenuAnchorPosition::kTopRight, source_type);
}

void MenuDelegate::ExecuteCommand(int id) {
  for (Observer& obs : observers_)
    obs.OnBeforeExecuteCommand();
  adapter_->ExecuteCommand(id);
}

void MenuDelegate::ExecuteCommand(int id, int mouse_event_flags) {
  for (Observer& obs : observers_)
    obs.OnBeforeExecuteCommand();
  adapter_->ExecuteCommand(id, mouse_event_flags);
}

bool MenuDelegate::IsTriggerableEvent(views::MenuItemView* source,
                                      const ui::Event& e) {
  return adapter_->IsTriggerableEvent(source, e);
}

bool MenuDelegate::GetAccelerator(int id, ui::Accelerator* accelerator) const {
  return adapter_->GetAccelerator(id, accelerator);
}

std::u16string MenuDelegate::GetLabel(int id) const {
  return adapter_->GetLabel(id);
}

const gfx::FontList* MenuDelegate::GetLabelFontList(int id) const {
  return adapter_->GetLabelFontList(id);
}

absl::optional<SkColor> MenuDelegate::GetLabelColor(int id) const {
  return adapter_->GetLabelColor(id);
}

bool MenuDelegate::IsCommandEnabled(int id) const {
  return adapter_->IsCommandEnabled(id);
}

bool MenuDelegate::IsCommandVisible(int id) const {
  return adapter_->IsCommandVisible(id);
}

bool MenuDelegate::IsItemChecked(int id) const {
  return adapter_->IsItemChecked(id);
}

void MenuDelegate::WillShowMenu(views::MenuItemView* menu) {
  adapter_->WillShowMenu(menu);
}

void MenuDelegate::WillHideMenu(views::MenuItemView* menu) {
  adapter_->WillHideMenu(menu);
}

void MenuDelegate::OnMenuClosed(views::MenuItemView* menu) {
  for (Observer& obs : observers_)
    obs.OnMenuClosed();

  // 仅在当前菜单关闭时切换到新菜单。
  if (button_to_open_)
    button_to_open_->Activate(nullptr);
  delete this;
}

views::MenuItemView* MenuDelegate::GetSiblingMenu(
    views::MenuItemView* menu,
    const gfx::Point& screen_point,
    views::MenuAnchorPosition* anchor,
    bool* has_mnemonics,
    views::MenuButton**) {
  if (hold_first_switch_) {
    hold_first_switch_ = false;
    return nullptr;
  }

  // TODO(Zcbenz)：我们应该遵循Chromium的逻辑来实现。
  // 兄弟菜单开关，这个代码几乎是一个黑客。
  views::MenuButton* button;
  ElectronMenuModel* model;
  if (menu_bar_->GetMenuButtonFromScreenPoint(screen_point, &model, &button) &&
      button->tag() != id_) {
    bool switch_in_progress = !!button_to_open_;
    // 始终将目标更新为打开。
    button_to_open_ = button;
    // 异步切换菜单，避免崩溃。
    if (!switch_in_progress) {
      base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                     base::BindOnce(&views::MenuRunner::Cancel,
                                    base::Unretained(menu_runner_.get())));
    }
  }

  return nullptr;
}

}  // 命名空间电子
