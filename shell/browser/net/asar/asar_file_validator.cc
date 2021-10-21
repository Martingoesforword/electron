// 版权所有(C)2021 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/net/asar/asar_file_validator.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "crypto/sha2.h"

namespace asar {

AsarFileValidator::AsarFileValidator(IntegrityPayload integrity,
                                     base::File file)
    : file_(std::move(file)), integrity_(std::move(integrity)) {
  current_block_ = 0;
  max_block_ = integrity_.blocks.size() - 1;
}

AsarFileValidator::~AsarFileValidator() = default;

void AsarFileValidator::OnRead(base::span<char> buffer,
                               mojo::FileDataSource::ReadResult* result) {
  DCHECK(!done_reading_);

  uint64_t buffer_size = result->bytes_read;

  // 计算我们应该散列的字节数，并将它们添加到当前散列中。
  uint32_t block_size = integrity_.block_size;
  uint64_t bytes_added = 0;
  while (bytes_added < buffer_size) {
    if (current_block_ > max_block_) {
      LOG(FATAL)
          << "Unexpected number of blocks while validating ASAR file stream";
      return;
    }

    // 如果我们还没有散列，请创建一个散列。
    if (!current_hash_) {
      current_hash_byte_count_ = 0;
      switch (integrity_.algorithm) {
        case HashAlgorithm::SHA256:
          current_hash_ =
              crypto::SecureHash::Create(crypto::SecureHash::SHA256);
          break;
        case HashAlgorithm::NONE:
          CHECK(false);
          break;
      }
    }

    // 计算我们应该散列的字节数，并将它们添加到当前散列中。
    // 我们需要添加刚好足以填满一个块的字节(block_size-。
    // CURRENT_BYTES)或使用剩余的每个字节(Buffer_Size-Bytes_Added)。
    int bytes_to_hash = std::min(block_size - current_hash_byte_count_,
                                 buffer_size - bytes_added);
    DCHECK_GT(bytes_to_hash, 0);
    current_hash_->Update(buffer.data() + bytes_added, bytes_to_hash);
    bytes_added += bytes_to_hash;
    current_hash_byte_count_ += bytes_to_hash;
    total_hash_byte_count_ += bytes_to_hash;

    if (current_hash_byte_count_ == block_size && !FinishBlock()) {
      LOG(FATAL) << "Failed to validate block while streaming ASAR file: "
                 << current_block_;
      return;
    }
  }
}

bool AsarFileValidator::FinishBlock() {
  if (current_hash_byte_count_ == 0) {
    if (!done_reading_ || current_block_ > max_block_) {
      return true;
    }
  }

  if (!current_hash_) {
    // 当我们无法读取资源时，就会发生这种情况。计算空内容%s。
    // 在本例中为散列。
    current_hash_ = crypto::SecureHash::Create(crypto::SecureHash::SHA256);
  }

  uint8_t actual[crypto::kSHA256Length];

  // 如果文件读取器已完成，我们需要确保已读取到。
  // 文件末尾(下面的检查)或最多到BLOCK_SIZE字节的末尾。
  // 边界。如果下面的检查失败，我们如何计算下一个块边界。
  // 需要很多字节才能到达那里，然后我们手动读取这些字节。
  // 从我们自己的文件句柄确保数据生产者不知道，但我们可以。
  // 仍然验证散列。
  if (done_reading_ &&
      total_hash_byte_count_ - extra_read_ != read_max_ - read_start_) {
    uint64_t bytes_needed = std::min(
        integrity_.block_size - current_hash_byte_count_,
        read_max_ - read_start_ - total_hash_byte_count_ + extra_read_);
    uint64_t offset = read_start_ + total_hash_byte_count_ - extra_read_;
    std::vector<uint8_t> abandoned_buffer(bytes_needed);
    if (!file_.ReadAndCheck(offset, abandoned_buffer)) {
      LOG(FATAL) << "Failed to read required portion of streamed ASAR archive";
      return false;
    }

    current_hash_->Update(&abandoned_buffer.front(), bytes_needed);
  }

  current_hash_->Finish(actual, sizeof(actual));
  current_hash_.reset();
  current_hash_byte_count_ = 0;

  const std::string expected_hash = integrity_.blocks[current_block_];
  const std::string actual_hex_hash =
      base::ToLowerASCII(base::HexEncode(actual, sizeof(actual)));

  if (expected_hash != actual_hex_hash) {
    return false;
  }

  current_block_++;

  return true;
}

void AsarFileValidator::OnDone() {
  DCHECK(!done_reading_);
  done_reading_ = true;
  if (!FinishBlock()) {
    LOG(FATAL) << "Failed to validate block while ending ASAR file stream: "
               << current_block_;
  }
}

void AsarFileValidator::SetRange(uint64_t read_start,
                                 uint64_t extra_read,
                                 uint64_t read_max) {
  read_start_ = read_start;
  extra_read_ = extra_read;
  read_max_ = read_max;
}

void AsarFileValidator::SetCurrentBlock(int current_block) {
  current_block_ = current_block;
}

}  // 命名空间asar
