// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/electron_command_line.h"

#include "base/command_line.h"
#include "uv.h"  // NOLINT(BUILD/INCLUDE_DIRECTORY)。

namespace electron {

// 静电。
base::CommandLine::StringVector ElectronCommandLine::argv_;

// 静电。
void ElectronCommandLine::Init(int argc, base::CommandLine::CharType** argv) {
  DCHECK(argv_.empty());

  // 注意：uv_setup_args在Windows上不执行任何操作，因此我们不需要调用它。
  // 否则，我们必须将参数从UTF16转换。
#if !defined(OS_WIN)
  // 拿着argv指针四处乱砍。用于进程.title=“废话”
  argv = uv_setup_args(argc, argv);
#endif

  argv_.assign(argv, argv + argc);
}

#if defined(OS_LINUX)
// 静电。
void ElectronCommandLine::InitializeFromCommandLine() {
  argv_ = base::CommandLine::ForCurrentProcess()->argv();
}
#endif

}  // 命名空间电子
