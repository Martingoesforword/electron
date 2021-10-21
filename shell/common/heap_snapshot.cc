// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/heap_snapshot.h"

#include "base/files/file.h"
#include "v8/include/v8-profiler.h"
#include "v8/include/v8.h"

namespace {

class HeapSnapshotOutputStream : public v8::OutputStream {
 public:
  explicit HeapSnapshotOutputStream(base::File* file) : file_(file) {
    DCHECK(file_);
  }

  bool IsComplete() const { return is_complete_; }

  // V8：：OutputStream。
  int GetChunkSize() override { return 65536; }
  void EndOfStream() override { is_complete_ = true; }

  v8::OutputStream::WriteResult WriteAsciiChunk(char* data, int size) override {
    auto bytes_written = file_->WriteAtCurrentPos(data, size);
    return bytes_written == size ? kContinue : kAbort;
  }

 private:
  base::File* file_ = nullptr;
  bool is_complete_ = false;
};

}  // 命名空间。

namespace electron {

bool TakeHeapSnapshot(v8::Isolate* isolate, base::File* file) {
  DCHECK(isolate);
  DCHECK(file);

  if (!file->IsValid())
    return false;

  auto* snapshot = isolate->GetHeapProfiler()->TakeHeapSnapshot();
  if (!snapshot)
    return false;

  HeapSnapshotOutputStream stream(file);
  snapshot->Serialize(&stream, v8::HeapSnapshot::kJSON);

  const_cast<v8::HeapSnapshot*>(snapshot)->Delete();

  return stream.IsComplete();
}

}  // 命名空间电子
