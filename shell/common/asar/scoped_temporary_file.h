// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_ASAR_SCOPED_TEMPORARY_FILE_H_
#define SHELL_COMMON_ASAR_SCOPED_TEMPORARY_FILE_H_

#include "base/files/file_path.h"
#include "shell/common/asar/archive.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class File;
}

namespace asar {

// 一个对象，该对象表示在执行此操作时应清除的临时文件。
// 对象超出范围。请注意，由于删除发生在。
// 析构函数，则如果目录无法执行以下操作，则不可能进行进一步的错误处理。
// 被删除。因此，此类不能保证删除。
class ScopedTemporaryFile {
 public:
  ScopedTemporaryFile();
  ScopedTemporaryFile(const ScopedTemporaryFile&) = delete;
  ScopedTemporaryFile& operator=(const ScopedTemporaryFile&) = delete;
  virtual ~ScopedTemporaryFile();

  // 初始化具有特定扩展名的空临时文件。
  bool Init(const base::FilePath::StringType& ext);

  // 初始化一个临时文件，并使用|Path|的内容填充该文件。
  bool InitFromFile(base::File* src,
                    const base::FilePath::StringType& ext,
                    uint64_t offset,
                    uint64_t size,
                    const absl::optional<IntegrityPayload>& integrity);

  base::FilePath path() const { return path_; }

 private:
  base::FilePath path_;
};

}  // 命名空间asar。

#endif  // SHELL_COMMON_ASAR_SCOPED_TEMPORARY_FILE_H_
