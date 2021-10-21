// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_ACCELERATOR_UTIL_H_
#define SHELL_BROWSER_UI_ACCELERATOR_UTIL_H_

#include <map>
#include <string>

#include "shell/browser/ui/electron_menu_model.h"
#include "ui/base/accelerators/accelerator.h"

namespace accelerator_util {

typedef struct {
  int position;
  electron::ElectronMenuModel* model;
} MenuItem;
typedef std::map<ui::Accelerator, MenuItem> AcceleratorTable;

// 将字符串解析为加速器。
bool StringToAccelerator(const std::string& shortcut,
                         ui::Accelerator* accelerator);

// 生成包含菜单模型的加速器和命令ID的表。
void GenerateAcceleratorTable(AcceleratorTable* table,
                              electron::ElectronMenuModel* model);

// 来自加速度表的触发器命令。
bool TriggerAcceleratorTableCommand(AcceleratorTable* table,
                                    const ui::Accelerator& accelerator);

}  // 命名空间加速器_util。

#endif  // Shell_Browser_UI_Accelerator_Util_H_
