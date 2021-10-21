// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/language_util.h"

#include "ui/base/l10n/l10n_util.h"

namespace electron {

std::vector<std::string> GetPreferredLanguages() {
  // 返回空，因为没有API可用。您或许可以使用。
  // 浏览器进程的GetApplicationLocale()。
  return std::vector<std::string>{};
}

}  // 命名空间电子
