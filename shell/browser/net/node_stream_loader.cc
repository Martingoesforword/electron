// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/net/node_stream_loader.h"

#include <utility>

#include "mojo/public/cpp/system/string_data_source.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/node_includes.h"

namespace electron {

NodeStreamLoader::NodeStreamLoader(
    network::mojom::URLResponseHeadPtr head,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    v8::Isolate* isolate,
    v8::Local<v8::Object> emitter)
    : url_loader_(this, std::move(loader)),
      client_(std::move(client)),
      isolate_(isolate),
      emitter_(isolate, emitter) {
  url_loader_.set_disconnect_handler(
      base::BindOnce(&NodeStreamLoader::NotifyComplete,
                     weak_factory_.GetWeakPtr(), net::ERR_FAILED));

  Start(std::move(head));
}

NodeStreamLoader::~NodeStreamLoader() {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);

  // 取消订阅所有处理程序。
  for (const auto& it : handlers_) {
    v8::Local<v8::Value> args[] = {gin::StringToV8(isolate_, it.first),
                                   it.second.Get(isolate_)};
    node::MakeCallback(isolate_, emitter_.Get(isolate_), "removeListener",
                       node::arraysize(args), args, {0, 0});
  }

  // 如果流尚未结束，请销毁该流。
  if (!ended_) {
    node::MakeCallback(isolate_, emitter_.Get(isolate_), "destroy", 0, nullptr,
                       {0, 0});
  }
}

void NodeStreamLoader::Start(network::mojom::URLResponseHeadPtr head) {
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  MojoResult rv = mojo::CreateDataPipe(nullptr, producer, consumer);
  if (rv != MOJO_RESULT_OK) {
    NotifyComplete(net::ERR_INSUFFICIENT_RESOURCES);
    return;
  }

  producer_ = std::make_unique<mojo::DataPipeProducer>(std::move(producer));
  client_->OnReceiveResponse(std::move(head));
  client_->OnStartLoadingResponseBody(std::move(consumer));

  auto weak = weak_factory_.GetWeakPtr();
  On("end",
     base::BindRepeating(&NodeStreamLoader::NotifyComplete, weak, net::OK));
  On("error", base::BindRepeating(&NodeStreamLoader::NotifyComplete, weak,
                                  net::ERR_FAILED));
  On("readable", base::BindRepeating(&NodeStreamLoader::NotifyReadable, weak));
}

void NodeStreamLoader::NotifyReadable() {
  if (!readable_)
    ReadMore();
  else if (is_reading_)
    has_read_waiting_ = true;
  readable_ = true;
}

void NodeStreamLoader::NotifyComplete(int result) {
  // 等待写入完成或失败。
  if (is_reading_ || is_writing_) {
    ended_ = true;
    result_ = result;
    return;
  }

  client_->OnComplete(network::URLLoaderCompletionStatus(result));
  delete this;
}

void NodeStreamLoader::ReadMore() {
  if (is_reading_) {
    // 调用read()可以再次触发“readable”事件，使得。
    // 函数重入。如果我们已经在阅读了，我们就不想开始。
    // 嵌套读取，所以短路。
    return;
  }
  is_reading_ = true;
  auto weak = weak_factory_.GetWeakPtr();
  v8::HandleScope scope(isolate_);
  // Buffer=emitter.read()。
  v8::MaybeLocal<v8::Value> ret = node::MakeCallback(
      isolate_, emitter_.Get(isolate_), "read", 0, nullptr, {0, 0});
  DCHECK(weak) << "We shouldn't have been destroyed when calling read()";

  // 如果没有缓冲区读取，请等待再次发出|Readable|。
  v8::Local<v8::Value> buffer;
  if (!ret.ToLocal(&buffer) || !node::Buffer::HasInstance(buffer)) {
    is_reading_ = false;

    // 如果在“read()”之后调用了“readable”，请重试。
    if (has_read_waiting_) {
      has_read_waiting_ = false;
      ReadMore();
      return;
    }

    readable_ = false;
    if (ended_) {
      NotifyComplete(result_);
    }
    return;
  }

  // 保持缓冲区，直到写入完成。
  buffer_.Reset(isolate_, buffer);

  // 将缓冲区异步写入mojo管道。
  is_reading_ = false;
  is_writing_ = true;
  producer_->Write(std::make_unique<mojo::StringDataSource>(
                       base::StringPiece(node::Buffer::Data(buffer),
                                         node::Buffer::Length(buffer)),
                       mojo::StringDataSource::AsyncWritingMode::
                           STRING_STAYS_VALID_UNTIL_COMPLETION),
                   base::BindOnce(&NodeStreamLoader::DidWrite, weak));
}

void NodeStreamLoader::DidWrite(MojoResult result) {
  is_writing_ = false;
  // 我们被告知停止流媒体播放。
  if (ended_) {
    NotifyComplete(result_);
    return;
  }

  if (result == MOJO_RESULT_OK && readable_)
    ReadMore();
  else
    NotifyComplete(net::ERR_FAILED);
}

void NodeStreamLoader::On(const char* event, EventCallback callback) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);

  // Emitter.on(事件，回调)。
  v8::Local<v8::Value> args[] = {
      gin::StringToV8(isolate_, event),
      gin_helper::CallbackToV8Leaked(isolate_, std::move(callback)),
  };
  handlers_[event].Reset(isolate_, args[1]);
  node::MakeCallback(isolate_, emitter_.Get(isolate_), "on",
                     node::arraysize(args), args, {0, 0});
  // 下面不再有代码，因为该类在订阅时可能会销毁。
}

}  // 命名空间电子
