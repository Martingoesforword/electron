// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_ELECTRON_COMMAND_LINE_H_
#define SHELL_COMMON_ELECTRON_COMMAND_LINE_H_

#include "base/command_line.h"
#include "base/macros.h"
#include "build/build_config.h"

namespace electron {

// Singleton记住原来的“argc”和“argv”。
class ElectronCommandLine {
 public:
  static const base::CommandLine::StringVector& argv() { return argv_; }

  static void Init(int argc, base::CommandLine::CharType** argv);

#if defined(OS_LINUX)
  // 在Linux上，命令行必须从base：：CommandLine读取，因为。
  // 它使用的是受精卵。
  static void InitializeFromCommandLine();
#endif

 private:
  static base::CommandLine::StringVector argv_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(ElectronCommandLine);
};

}  // 命名空间电子。

#endif  // Shell_COMMON_ELECT_COMMAND_LINE_H_
