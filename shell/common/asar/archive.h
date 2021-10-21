// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_ASAR_ARCHIVE_H_
#define SHELL_COMMON_ASAR_ARCHIVE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/synchronization/lock.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class DictionaryValue;
}

namespace asar {

class ScopedTemporaryFile;

enum HashAlgorithm {
  SHA256,
  NONE,
};

struct IntegrityPayload {
  IntegrityPayload();
  ~IntegrityPayload();
  IntegrityPayload(const IntegrityPayload& other);
  HashAlgorithm algorithm;
  std::string hash;
  uint32_t block_size;
  std::vector<std::string> blocks;
};

// 此类表示asar包，并提供读取。
// 其中的信息。在|Init|被调用之后，它是线程安全的。
class Archive {
 public:
  struct FileInfo {
    FileInfo();
    ~FileInfo();
    bool unpacked;
    bool executable;
    uint32_t size;
    uint64_t offset;
    absl::optional<IntegrityPayload> integrity;
  };

  struct Stats : public FileInfo {
    Stats() : is_file(true), is_directory(false), is_link(false) {}
    bool is_file;
    bool is_directory;
    bool is_link;
  };

  explicit Archive(const base::FilePath& path);
  virtual ~Archive();

  // 读取并解析标题。
  bool Init();

  absl::optional<IntegrityPayload> HeaderIntegrity() const;
  absl::optional<base::FilePath> RelativePath() const;

  // 获取文件的信息。
  bool GetFileInfo(const base::FilePath& path, FileInfo* info) const;

  // Fs.stat(路径)。
  bool Stat(const base::FilePath& path, Stats* stats) const;

  // Fs.readdir(路径)。
  bool Readdir(const base::FilePath& path,
               std::vector<base::FilePath>* files) const;

  // Fs.realpath(路径)。
  bool Realpath(const base::FilePath& path, base::FilePath* realpath) const;

  // 将文件复制到临时文件中，并返回新路径。
  // 对于解压缩的文件，此方法将返回其真实路径。
  bool CopyFileOut(const base::FilePath& path, base::FilePath* out);

  // 返回文件的FD。
  // 使用此FD不会验证任何文件的完整性。
  // 您可以手动读出ASAR。打电话的人要负责。
  // 用于此FD移交后的完整性验证。
  int GetUnsafeFD() const;

  base::FilePath path() const { return path_; }

 private:
  bool initialized_;
  bool header_validated_ = false;
  const base::FilePath path_;
  base::File file_;
  int fd_ = -1;
  uint32_t header_size_ = 0;
  std::unique_ptr<base::DictionaryValue> header_;

  // 缓存的外部临时文件。
  base::Lock external_files_lock_;
  std::unordered_map<base::FilePath::StringType,
                     std::unique_ptr<ScopedTemporaryFile>>
      external_files_;

  DISALLOW_COPY_AND_ASSIGN(Archive);
};

}  // 命名空间asar。

#endif  // SHELL_COMMON_ASAR_ARCHIVE_H_
