// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NET_NODE_STREAM_LOADER_H_
#define SHELL_BROWSER_NET_NODE_STREAM_LOADER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "v8/include/v8.h"

namespace electron {

// 从Node Stream读取数据并将其提供给NetworkService。
// 
// 此类管理其自身的生存期，并应在。
// 连接丢失或完成。
// 
// 我们使用|暂停模式|从|可读|流中读取数据，因此不需要。
// 从缓冲区复制数据并将其保存在内存中，我们只需要确保。
// 将数据写入管道时，传递的|Buffer|是活动的。
class NodeStreamLoader : public network::mojom::URLLoader {
 public:
  NodeStreamLoader(network::mojom::URLResponseHeadPtr head,
                   mojo::PendingReceiver<network::mojom::URLLoader> loader,
                   mojo::PendingRemote<network::mojom::URLLoaderClient> client,
                   v8::Isolate* isolate,
                   v8::Local<v8::Object> emitter);

 private:
  ~NodeStreamLoader() override;

  using EventCallback = base::RepeatingCallback<void()>;

  void Start(network::mojom::URLResponseHeadPtr head);
  void NotifyReadable();
  void NotifyComplete(int result);
  void ReadMore();
  void DidWrite(MojoResult result);

  // 订阅|发射器|的活动。
  void On(const char* event, EventCallback callback);

  // URLLoader：
  void FollowRedirect(
      const std::vector<std::string>& removed_headers,
      const net::HttpRequestHeaders& modified_headers,
      const net::HttpRequestHeaders& modified_cors_exempt_headers,
      const absl::optional<GURL>& new_url) override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

  mojo::Receiver<network::mojom::URLLoader> url_loader_;
  mojo::Remote<network::mojom::URLLoaderClient> client_;

  v8::Isolate* isolate_;
  v8::Global<v8::Object> emitter_;
  v8::Global<v8::Value> buffer_;

  // 正在读取的数据写入的MOJO数据管道。
  std::unique_ptr<mojo::DataPipeProducer> producer_;

  // 我们是否在写到一半。
  bool is_writing_ = false;

  // 我们是否在流的中间。read()。
  bool is_reading_ = false;

  // 当在写入过程中调用NotifyComplete时，我们将保存结果并。
  // 在写入完成后使用它退出。
  bool ended_ = false;
  int result_ = net::OK;

  // 当流发出Readable事件时，我们只想开始读取。
  // 数据(如果流以前不可读)，因此我们将状态存储在。
  // 旗帜。
  bool readable_ = false;

  // 可以在read()过程中使用nextTick()对读取进行排队。
  // 这将导致在Readmore过程中发出“可读性”，因此我们跟踪。
  // 出现在一面旗帜上。
  bool has_read_waiting_ = false;

  // 存储V8回调，以便稍后取消订阅。
  std::map<std::string, v8::Global<v8::Value>> handlers_;

  base::WeakPtrFactory<NodeStreamLoader> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(NodeStreamLoader);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Net_Node_Stream_Loader_H_
