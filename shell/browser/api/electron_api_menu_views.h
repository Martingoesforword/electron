// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_MENU_VIEWS_H_
#define SHELL_BROWSER_API_ELECTRON_API_MENU_VIEWS_H_

#include <map>
#include <memory>

#include "base/memory/weak_ptr.h"
#include "shell/browser/api/electron_api_menu.h"
#include "ui/display/screen.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace electron {

namespace api {

class MenuViews : public Menu {
 public:
  explicit MenuViews(gin::Arguments* args);
  ~MenuViews() override;

 protected:
  void PopupAt(BaseWindow* window,
               int x,
               int y,
               int positioning_item,
               base::OnceClosure callback) override;
  void ClosePopupAt(int32_t window_id) override;

 private:
  void OnClosed(int32_t window_id, base::OnceClosure callback);

  // 窗口ID-&gt;打开上下文菜单。
  std::map<int32_t, std::unique_ptr<views::MenuRunner>> menu_runners_;

  base::WeakPtrFactory<MenuViews> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(MenuViews);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Electronics_API_Menu_Views_H_
