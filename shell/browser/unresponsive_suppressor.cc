// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/unresponsive_suppressor.h"

namespace electron {

namespace {

int g_suppress_level = 0;

}  // 命名空间。

bool IsUnresponsiveEventSuppressed() {
  return g_suppress_level > 0;
}

UnresponsiveSuppressor::UnresponsiveSuppressor() {
  g_suppress_level++;
}

UnresponsiveSuppressor::~UnresponsiveSuppressor() {
  g_suppress_level--;
}

}  // 命名空间电子
