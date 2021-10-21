// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_data_pipe_holder.h"

#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/base/net_errors.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/key_weak_map.h"

#include "shell/common/node_includes.h"

namespace electron {

namespace api {

namespace {

// 增量ID。
int g_next_id = 0;

// 管理所有DataPipeHolder对象的地图。
KeyWeakMap<std::string>& AllDataPipeHolders() {
  static base::NoDestructor<KeyWeakMap<std::string>> weak_map;
  return *weak_map.get();
}

// 要从数据管道读取的实用程序类。
class DataPipeReader {
 public:
  DataPipeReader(gin_helper::Promise<v8::Local<v8::Value>> promise,
                 mojo::Remote<network::mojom::DataPipeGetter> data_pipe_getter)
      : promise_(std::move(promise)),
        data_pipe_getter_(std::move(data_pipe_getter)),
        handle_watcher_(FROM_HERE,
                        mojo::SimpleWatcher::ArmingPolicy::MANUAL,
                        base::SequencedTaskRunnerHandle::Get()) {
    // 找一条新的数据管道，然后开始。
    mojo::ScopedDataPipeProducerHandle producer_handle;
    CHECK_EQ(mojo::CreateDataPipe(nullptr, producer_handle, data_pipe_),
             MOJO_RESULT_OK);
    data_pipe_getter_->Read(std::move(producer_handle),
                            base::BindOnce(&DataPipeReader::ReadCallback,
                                           weak_factory_.GetWeakPtr()));
    handle_watcher_.Watch(data_pipe_.get(), MOJO_HANDLE_SIGNAL_READABLE,
                          base::BindRepeating(&DataPipeReader::OnHandleReadable,
                                              weak_factory_.GetWeakPtr()));
  }

  ~DataPipeReader() = default;

 private:
  // 由DataPipeGetter：：Read调用的回调。
  void ReadCallback(int32_t status, uint64_t size) {
    if (status != net::OK) {
      OnFailure();
      return;
    }
    buffer_.resize(size);
    head_ = &buffer_.front();
    remaining_size_ = size;
    handle_watcher_.ArmOrNotify();
  }

  // 当数据可用或管道关闭时，由|HANDLE_WATCHER_|调用。
  // 还有一个挂起的read()调用。
  void OnHandleReadable(MojoResult result) {
    if (result != MOJO_RESULT_OK) {
      OnFailure();
      return;
    }

    // 朗读。
    uint32_t length = remaining_size_;
    result = data_pipe_->ReadData(head_, &length, MOJO_READ_DATA_FLAG_NONE);
    if (result == MOJO_RESULT_OK) {  // 成功。
      remaining_size_ -= length;
      head_ += length;
      if (remaining_size_ == 0)
        OnSuccess();
    } else if (result == MOJO_RESULT_SHOULD_WAIT) {  // IO挂起。
      handle_watcher_.ArmOrNotify();
    } else {  // 错误。
      OnFailure();
    }
  }

  void OnFailure() {
    promise_.RejectWithErrorMessage("Could not get blob data");
    delete this;
  }

  void OnSuccess() {
    // 将缓冲区传递给JS。
    // 
    // 请注意，本机缓冲区的生存期属于我们，我们将。
    // 当JS缓冲区被垃圾回收时释放内存。
    v8::Locker locker(promise_.isolate());
    v8::HandleScope handle_scope(promise_.isolate());
    v8::Local<v8::Value> buffer =
        node::Buffer::New(promise_.isolate(), &buffer_.front(), buffer_.size(),
                          &DataPipeReader::FreeBuffer, this)
            .ToLocalChecked();
    promise_.Resolve(buffer);

    // 摧毁数据管道。
    handle_watcher_.Cancel();
    data_pipe_.reset();
    data_pipe_getter_.reset();
  }

  static void FreeBuffer(char* data, void* self) {
    delete static_cast<DataPipeReader*>(self);
  }

  gin_helper::Promise<v8::Local<v8::Value>> promise_;

  mojo::Remote<network::mojom::DataPipeGetter> data_pipe_getter_;
  mojo::ScopedDataPipeConsumerHandle data_pipe_;
  mojo::SimpleWatcher handle_watcher_;

  // 存储读取的数据。
  std::vector<char> buffer_;

  // 缓冲区的头。
  char* head_ = nullptr;

  // 要读取的剩余数据。
  uint64_t remaining_size_ = 0;

  base::WeakPtrFactory<DataPipeReader> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(DataPipeReader);
};

}  // 命名空间。

gin::WrapperInfo DataPipeHolder::kWrapperInfo = {gin::kEmbedderNativeGin};

DataPipeHolder::DataPipeHolder(const network::DataElement& element)
    : id_(base::NumberToString(++g_next_id)) {
  data_pipe_.Bind(
      element.As<network::DataElementDataPipe>().CloneDataPipeGetter());
}

DataPipeHolder::~DataPipeHolder() = default;

v8::Local<v8::Promise> DataPipeHolder::ReadAll(v8::Isolate* isolate) {
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  if (!data_pipe_) {
    promise.RejectWithErrorMessage("Could not get blob data");
    return handle;
  }

  new DataPipeReader(std::move(promise), std::move(data_pipe_));
  return handle;
}

// 静电。
gin::Handle<DataPipeHolder> DataPipeHolder::Create(
    v8::Isolate* isolate,
    const network::DataElement& element) {
  auto handle = gin::CreateHandle(isolate, new DataPipeHolder(element));
  AllDataPipeHolders().Set(isolate, handle->id(),
                           handle->GetWrapper(isolate).ToLocalChecked());
  return handle;
}

// 静电。
gin::Handle<DataPipeHolder> DataPipeHolder::From(v8::Isolate* isolate,
                                                 const std::string& id) {
  v8::MaybeLocal<v8::Object> object = AllDataPipeHolders().Get(isolate, id);
  if (!object.IsEmpty()) {
    gin::Handle<DataPipeHolder> handle;
    if (gin::ConvertFromV8(isolate, object.ToLocalChecked(), &handle))
      return handle;
  }
  return gin::Handle<DataPipeHolder>();
}

}  // 命名空间API。

}  // 命名空间电子
