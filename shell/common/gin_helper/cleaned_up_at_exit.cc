// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_helper/cleaned_up_at_exit.h"

#include <algorithm>
#include <vector>

#include "base/no_destructor.h"

namespace gin_helper {

std::vector<CleanedUpAtExit*>& GetDoomed() {
  static base::NoDestructor<std::vector<CleanedUpAtExit*>> doomed;
  return *doomed;
}
CleanedUpAtExit::CleanedUpAtExit() {
  GetDoomed().emplace_back(this);
}
CleanedUpAtExit::~CleanedUpAtExit() {
  auto& doomed = GetDoomed();
  doomed.erase(std::remove(doomed.begin(), doomed.end(), this), doomed.end());
}

// 静电。
void CleanedUpAtExit::DoCleanup() {
  auto& doomed = GetDoomed();
  while (!doomed.empty()) {
    CleanedUpAtExit* next = doomed.back();
    delete next;
  }
}

}  // 命名空间gin_helper
