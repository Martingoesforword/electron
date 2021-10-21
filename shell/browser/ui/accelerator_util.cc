// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/accelerator_util.h"

#include <stdio.h>

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "shell/common/keyboard_util.h"

namespace accelerator_util {

bool StringToAccelerator(const std::string& shortcut,
                         ui::Accelerator* accelerator) {
  if (!base::IsStringASCII(shortcut)) {
    LOG(ERROR) << "The accelerator string can only contain ASCII characters";
    return false;
  }

  std::vector<std::string> tokens = base::SplitString(
      shortcut, "+", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  // 现在，把它解析成一个加速器。
  int modifiers = ui::EF_NONE;
  ui::KeyboardCode key = ui::VKEY_UNKNOWN;
  absl::optional<char16_t> shifted_char;
  for (const auto& token : tokens) {
    ui::KeyboardCode code = electron::KeyboardCodeFromStr(token, &shifted_char);
    if (shifted_char)
      modifiers |= ui::EF_SHIFT_DOWN;
    switch (code) {
      // 令牌可以是修饰语。
      case ui::VKEY_SHIFT:
        modifiers |= ui::EF_SHIFT_DOWN;
        break;
      case ui::VKEY_CONTROL:
        modifiers |= ui::EF_CONTROL_DOWN;
        break;
      case ui::VKEY_MENU:
        modifiers |= ui::EF_ALT_DOWN;
        break;
      case ui::VKEY_COMMAND:
        modifiers |= ui::EF_COMMAND_DOWN;
        break;
      case ui::VKEY_ALTGR:
        modifiers |= ui::EF_ALTGR_DOWN;
        break;
      // 或者这是一把普通钥匙。
      default:
        key = code;
    }
  }

  if (key == ui::VKEY_UNKNOWN) {
    LOG(WARNING) << shortcut << " doesn't contain a valid key";
    return false;
  }

  *accelerator = ui::Accelerator(key, modifiers);
  accelerator->shifted_char = shifted_char;
  return true;
}

void GenerateAcceleratorTable(AcceleratorTable* table,
                              electron::ElectronMenuModel* model) {
  int count = model->GetItemCount();
  for (int i = 0; i < count; ++i) {
    electron::ElectronMenuModel::ItemType type = model->GetTypeAt(i);
    if (type == electron::ElectronMenuModel::TYPE_SUBMENU) {
      auto* submodel = model->GetSubmenuModelAt(i);
      GenerateAcceleratorTable(table, submodel);
    } else {
      ui::Accelerator accelerator;
      if (model->ShouldRegisterAcceleratorAt(i)) {
        if (model->GetAcceleratorAtWithParams(i, true, &accelerator)) {
          MenuItem item = {i, model};
          (*table)[accelerator] = item;
        }
      }
    }
  }
}

bool TriggerAcceleratorTableCommand(AcceleratorTable* table,
                                    const ui::Accelerator& accelerator) {
  const auto iter = table->find(accelerator);
  if (iter != std::end(*table)) {
    const accelerator_util::MenuItem& item = iter->second;
    if (item.model->IsEnabledAt(item.position)) {
      const auto event_flags =
          accelerator.MaskOutKeyEventFlags(accelerator.modifiers());
      item.model->ActivatedAt(item.position, event_flags);
      return true;
    }
  }
  return false;
}

}  // 命名空间加速器_util
