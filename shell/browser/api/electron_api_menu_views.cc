// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_menu_views.h"

#include <memory>
#include <utility>

#include "shell/browser/native_window_views.h"
#include "shell/browser/unresponsive_suppressor.h"
#include "ui/display/screen.h"

using views::MenuRunner;

namespace electron {

namespace api {

MenuViews::MenuViews(gin::Arguments* args) : Menu(args) {}

MenuViews::~MenuViews() = default;

void MenuViews::PopupAt(BaseWindow* window,
                        int x,
                        int y,
                        int positioning_item,
                        base::OnceClosure callback) {
  auto* native_window = static_cast<NativeWindowViews*>(window->window());
  if (!native_window)
    return;

  // (-1，-1)表示在鼠标位置上显示。
  gfx::Point location;
  if (x == -1 || y == -1) {
    location = display::Screen::GetScreen()->GetCursorScreenPoint();
  } else {
    gfx::Point origin = native_window->GetContentBounds().origin();
    location = gfx::Point(origin.x() + x, origin.y() + y);
  }

  int flags = MenuRunner::CONTEXT_MENU | MenuRunner::HAS_MNEMONICS;

  // 显示菜单时不发出无响应事件。
  electron::UnresponsiveSuppressor suppressor;

  // 确保在回调之前不会对菜单对象进行垃圾回收。
  // 已经跑了。
  base::OnceClosure callback_with_ref = BindSelfToClosure(std::move(callback));

  // 请出示菜单。
  // 
  // 请注意，虽然Views：：MenuRunner接受RepeatingCallback作为关闭。
  // 回调，可以将OnceCallback传递给它，因为我们将。
  // 菜单关闭时立即启动菜单运行器。
  int32_t window_id = window->weak_map_id();
  auto close_callback = base::AdaptCallbackForRepeating(
      base::BindOnce(&MenuViews::OnClosed, weak_factory_.GetWeakPtr(),
                     window_id, std::move(callback_with_ref)));
  menu_runners_[window_id] =
      std::make_unique<MenuRunner>(model(), flags, std::move(close_callback));
  menu_runners_[window_id]->RunMenuAt(
      native_window->widget(), nullptr, gfx::Rect(location, gfx::Size()),
      views::MenuAnchorPosition::kTopLeft, ui::MENU_SOURCE_MOUSE);
}

void MenuViews::ClosePopupAt(int32_t window_id) {
  auto runner = menu_runners_.find(window_id);
  if (runner != menu_runners_.end()) {
    // 合上窗户的流道。
    runner->second->Cancel();
  } else if (window_id == -1) {
    // 或者干脆关闭所有打开的跑道。
    for (auto it = menu_runners_.begin(); it != menu_runners_.end();) {
      // 在调用之后，迭代器无效。
      (it++)->second->Cancel();
    }
  }
}

void MenuViews::OnClosed(int32_t window_id, base::OnceClosure callback) {
  menu_runners_.erase(window_id);
  std::move(callback).Run();
}

// 静电。
gin::Handle<Menu> Menu::New(gin::Arguments* args) {
  auto handle = gin::CreateHandle(args->isolate(),
                                  static_cast<Menu*>(new MenuViews(args)));
  gin_helper::CallMethod(args->isolate(), handle.get(), "_init");
  return handle;
}

}  // 命名空间API。

}  // 命名空间电子
