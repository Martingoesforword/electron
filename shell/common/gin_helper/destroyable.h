// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_DESTROYABLE_H_
#define SHELL_COMMON_GIN_HELPER_DESTROYABLE_H_

#include "v8/include/v8.h"

namespace gin_helper {

// 管理包装在JS包装器中的本机对象。
struct Destroyable {
  // 确定本机对象是否已销毁。
  static bool IsDestroyed(v8::Local<v8::Object> object);

  // 在原型链中添加“销毁”和“isDestroed”。
  static void MakeDestroyable(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype);
};

}  // 命名空间gin_helper。

#endif  // SHELL_COMMON_GIN_HELPER_DESTORYABLE_H_
