// 版权所有(C)2021 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NET_ASAR_ASAR_FILE_VALIDATOR_H_
#define SHELL_BROWSER_NET_ASAR_ASAR_FILE_VALIDATOR_H_

#include <algorithm>
#include <memory>

#include "crypto/secure_hash.h"
#include "mojo/public/cpp/system/file_data_source.h"
#include "mojo/public/cpp/system/filtered_data_source.h"
#include "shell/common/asar/archive.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace asar {

class AsarFileValidator : public mojo::FilteredDataSource::Filter {
 public:
  AsarFileValidator(IntegrityPayload integrity, base::File file);
  ~AsarFileValidator() override;

  void OnRead(base::span<char> buffer,
              mojo::FileDataSource::ReadResult* result) override;

  void OnDone() override;

  void SetRange(uint64_t read_start, uint64_t extra_read, uint64_t read_max);
  void SetCurrentBlock(int current_block);

 protected:
  bool FinishBlock();

 private:
  base::File file_;
  IntegrityPayload integrity_;

  // 基础文件读取器开始的file_中的偏移量。
  uint64_t read_start_ = 0;
  // 此DataSourceFilter将看到的未使用的字节数。
  // 由DataProducer创建。这些额外的字节专门用于散列验证。
  // 但我们需要知道我们已经用了多少，这样我们才能知道我们什么时候用完了。
  uint64_t extra_read_ = 0;
  // 文件中我们应该读取的最大偏移量，用于确定。
  // 我们遗漏了哪些字节，或者我们是否需要读取到中的块边界。
  // 完成。
  uint64_t read_max_ = 0;
  bool done_reading_ = false;
  int current_block_;
  int max_block_;
  uint64_t current_hash_byte_count_ = 0;
  uint64_t total_hash_byte_count_ = 0;
  std::unique_ptr<crypto::SecureHash> current_hash_;

  DISALLOW_COPY_AND_ASSIGN(AsarFileValidator);
};

}  // 命名空间asar。

#endif  // Shell_Browser_Net_ASAR_ASAR_FILE_VALIDATOR_H_
