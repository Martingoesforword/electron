// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/platform_util.h"

#include <utility>

#include "base/bind.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/common/platform_util_internal.h"

namespace platform_util {

struct TrashItemResult {
  bool success;
  std::string error;
};

TrashItemResult TrashItemOnBlockingThread(const base::FilePath& full_path) {
  std::string error;
  bool success = internal::PlatformTrashItem(full_path, &error);
  return {success, error};
}

void TrashItem(const base::FilePath& full_path,
               base::OnceCallback<void(bool, const std::string&)> callback) {
  // XXX：CONTINUE_ON_SHUTDOWN对吗？
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&TrashItemOnBlockingThread, full_path),
      base::BindOnce(
          [](base::OnceCallback<void(bool, const std::string&)> callback,
             TrashItemResult result) {
            std::move(callback).Run(result.success, result.error);
          },
          std::move(callback)));
}

}  // 命名空间Platform_util
