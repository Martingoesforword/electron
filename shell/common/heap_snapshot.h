// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_HEAP_SNAPSHOT_H_
#define SHELL_COMMON_HEAP_SNAPSHOT_H_

namespace base {
class File;
}

namespace v8 {
class Isolate;
}

namespace electron {

bool TakeHeapSnapshot(v8::Isolate* isolate, base::File* file);

}  // 命名空间电子。

#endif  // SHELL_COMMON_HEAP_SNAPSHOT_H_
