// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在LICENSE.Cr文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_LOCKER_H_
#define SHELL_COMMON_GIN_HELPER_LOCKER_H_

#include <memory>

#include "base/macros.h"
#include "v8/include/v8.h"

namespace gin_helper {

// 只有在当前线程中使用锁柜时才会锁定。
class Locker {
 public:
  explicit Locker(v8::Isolate* isolate);
  ~Locker();

  // 返回当前进程是否为浏览器进程，当前我们检测到它。
  // 通过检查Current是否使用了V8 Lock，但这可能不是一个好主意。
  static inline bool IsBrowserProcess() { return v8::Locker::IsActive(); }

 private:
  void* operator new(size_t size);
  void operator delete(void*, size_t);

  std::unique_ptr<v8::Locker> locker_;

  DISALLOW_COPY_AND_ASSIGN(Locker);
};

}  // 命名空间gin_helper。

#endif  // SHELL_COMMON_GIN_HELPER_LOCKER_H_
