// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NET_URL_PIPE_LOADER_H_
#define SHELL_BROWSER_NET_URL_PIPE_LOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace network {
class SharedURLLoaderFactory;
}

namespace electron {

// 从URL读取数据并通过管道将其传输到NetworkService。
// 
// 与直接为URL创建新的加载器不同，协议处理程序。
// 使用此加载器可以绕过CORS限制。
// 
// 此类管理其自身的生存期，并应在。
// 连接丢失或完成。
class URLPipeLoader : public network::mojom::URLLoader,
                      public network::SimpleURLLoaderStreamConsumer {
 public:
  URLPipeLoader(scoped_refptr<network::SharedURLLoaderFactory> factory,
                std::unique_ptr<network::ResourceRequest> request,
                mojo::PendingReceiver<network::mojom::URLLoader> loader,
                mojo::PendingRemote<network::mojom::URLLoaderClient> client,
                const net::NetworkTrafficAnnotationTag& annotation,
                base::DictionaryValue upload_data);

 private:
  ~URLPipeLoader() override;

  void Start(scoped_refptr<network::SharedURLLoaderFactory> factory,
             std::unique_ptr<network::ResourceRequest> request,
             const net::NetworkTrafficAnnotationTag& annotation,
             base::DictionaryValue upload_data);
  void NotifyComplete(int result);
  void OnResponseStarted(const GURL& final_url,
                         const network::mojom::URLResponseHead& response_head);
  void OnWrite(base::OnceClosure resume, MojoResult result);

  // SimpleURLLoaderStreamConsumer：
  void OnDataReceived(base::StringPiece string_piece,
                      base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override;

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

  std::unique_ptr<mojo::DataPipeProducer> producer_;
  std::unique_ptr<network::SimpleURLLoader> loader_;

  base::WeakPtrFactory<URLPipeLoader> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(URLPipeLoader);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Net_URL_PIPE_LOADER_H_
