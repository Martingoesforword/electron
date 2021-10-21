// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/accelerator_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace accelerator_util {

TEST(AcceleratorUtilTest, StringToAccelerator) {
  struct {
    const std::string& description;
    bool expected_success;
  } keys[] = {
      {"♫♫♫♫♫♫♫", false},   {"Cmd+Plus", true}, {"Ctrl+Space", true},
      {"CmdOrCtrl", false}, {"Alt+Tab", true},  {"AltGr+Backspace", true},
      {"Super+Esc", true},  {"Super+X", true},  {"Shift+1", true},
  };

  for (const auto& key : keys) {
    // 初始化空但不为空的加速器。
    ui::Accelerator out = ui::Accelerator(ui::VKEY_UNKNOWN, ui::EF_NONE);
    bool success = StringToAccelerator(key.description, &out);
    EXPECT_EQ(success, key.expected_success);
  }
}

}  // 命名空间加速器_util
