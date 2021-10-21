// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/views/menu_model_adapter.h"

namespace electron {

MenuModelAdapter::MenuModelAdapter(ElectronMenuModel* menu_model)
    : views::MenuModelAdapter(menu_model), menu_model_(menu_model) {}

MenuModelAdapter::~MenuModelAdapter() = default;

bool MenuModelAdapter::GetAccelerator(int id,
                                      ui::Accelerator* accelerator) const {
  ui::MenuModel* model = menu_model_;
  int index = 0;
  if (ui::MenuModel::GetModelAndIndexForCommandId(id, &model, &index)) {
    return static_cast<ElectronMenuModel*>(model)->GetAcceleratorAtWithParams(
        index, true, accelerator);
  }
  return false;
}

}  // 命名空间电子
