// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UNRESPONSIVE_SUPPRESSOR_H_
#define SHELL_BROWSER_UNRESPONSIVE_SUPPRESSOR_H_

#include "base/macros.h"

namespace electron {

bool IsUnresponsiveEventSuppressed();

class UnresponsiveSuppressor {
 public:
  UnresponsiveSuppressor();
  ~UnresponsiveSuppressor();

 private:
  DISALLOW_COPY_AND_ASSIGN(UnresponsiveSuppressor);
};

}  // 命名空间电子。

#endif  // 外壳浏览器_无响应_抑制器_H_
