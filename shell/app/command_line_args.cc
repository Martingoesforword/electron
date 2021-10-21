// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/app/command_line_args.h"

#include <locale>

#include "sandbox/policy/switches.h"
#include "shell/common/options_switches.h"

namespace {

bool IsUrlArg(const base::CommandLine::CharType* arg) {
  // 第一个字符必须是字母，这才是URL。
  auto c = *arg;
  if (std::isalpha(c, std::locale::classic())) {
    for (auto* p = arg + 1; *p; ++p) {
      c = *p;

      // 冒号表示参数以URI方案开头。
      if (c == ':') {
        // 它也可以是Windows文件系统路径。
        if (p == arg + 1)
          break;

        return true;
      }

      // 冒号前的空格表示它不是URL。
      if (std::isspace(c, std::locale::classic()))
        break;
    }
  }

  return false;
}

}  // 命名空间。

namespace electron {

bool CheckCommandLineArguments(int argc, base::CommandLine::CharType** argv) {
  const base::CommandLine::StringType dashdash(2, '-');
  bool block_args = false;
  for (int i = 0; i < argc; ++i) {
    if (argv[i] == dashdash)
      break;
    if (block_args) {
      return false;
    } else if (IsUrlArg(argv[i])) {
      block_args = true;
    }
  }
  return true;
}

bool IsSandboxEnabled(base::CommandLine* command_line) {
  return command_line->HasSwitch(switches::kEnableSandbox) ||
         !command_line->HasSwitch(sandbox::policy::switches::kNoSandbox);
}

}  // 命名空间电子
