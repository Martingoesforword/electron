// 版权所有(C)2020塞缪尔·马多克。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_WORLD_IDS_H_
#define SHELL_COMMON_WORLD_IDS_H_

#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"  // 点名检查。

namespace electron {

enum WorldIDs : int32_t {
  MAIN_WORLD_ID = 0,

  // 使用远离0的较大数字可不与任何其他世界发生冲突。
  // Chrome内部创建的ID。
  ISOLATED_WORLD_ID = 999,

  // 扩展的隔离世界的数字大于或等于。
  // 此数字，最高可达ISOLATED_WORLD_ID_EXTENSIONS_END。
  ISOLATED_WORLD_ID_EXTENSIONS = 1 << 20,

  // 最后一个有效的隔离世界ID。
  ISOLATED_WORLD_ID_EXTENSIONS_END =
      blink::IsolatedWorldId::kEmbedderWorldIdLimit - 1
};

}  // 命名空间电子。

#endif  // SHELL_COMMON_WORLD_IDS_H_
