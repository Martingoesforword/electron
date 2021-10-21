// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_MENU_MODEL_ADAPTER_H_
#define SHELL_BROWSER_UI_VIEWS_MENU_MODEL_ADAPTER_H_

#include "shell/browser/ui/electron_menu_model.h"
#include "ui/views/controls/menu/menu_model_adapter.h"

namespace electron {

class MenuModelAdapter : public views::MenuModelAdapter {
 public:
  explicit MenuModelAdapter(ElectronMenuModel* menu_model);
  ~MenuModelAdapter() override;

 protected:
  bool GetAccelerator(int id, ui::Accelerator* accelerator) const override;

 private:
  ElectronMenuModel* menu_model_;

  DISALLOW_COPY_AND_ASSIGN(MenuModelAdapter);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Views_Menu_Model_Adapter_H_
