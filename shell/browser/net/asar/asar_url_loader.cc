// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/net/asar/asar_url_loader.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/file_url_loader.h"
#include "electron/fuses.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe_producer.h"
#include "mojo/public/cpp/system/file_data_source.h"
#include "net/base/filename_util.h"
#include "net/base/mime_sniffer.h"
#include "net/base/mime_util.h"
#include "net/http/http_byte_range.h"
#include "net/http/http_util.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "shell/browser/net/asar/asar_file_validator.h"
#include "shell/common/asar/archive.h"
#include "shell/common/asar/asar_util.h"

namespace asar {

namespace {

net::Error ConvertMojoResultToNetError(MojoResult result) {
  switch (result) {
    case MOJO_RESULT_OK:
      return net::OK;
    case MOJO_RESULT_NOT_FOUND:
      return net::ERR_FILE_NOT_FOUND;
    case MOJO_RESULT_PERMISSION_DENIED:
      return net::ERR_ACCESS_DENIED;
    case MOJO_RESULT_RESOURCE_EXHAUSTED:
      return net::ERR_INSUFFICIENT_RESOURCES;
    case MOJO_RESULT_ABORTED:
      return net::ERR_ABORTED;
    default:
      return net::ERR_FAILED;
  }
}

constexpr size_t kDefaultFileUrlPipeSize = 65536;

// 因为这让事情变得简单了。
static_assert(kDefaultFileUrlPipeSize >= net::kMaxBytesToSniff,
              "Default file data pipe size must be at least as large as a MIME-"
              "type sniffing buffer.");

// 从|file_url_loader_factory.cc|中的|FileURLLoader|修改，以提供。
// ASAR文件，而不是普通文件。
class AsarURLLoader : public network::mojom::URLLoader {
 public:
  static void CreateAndStart(
      const network::ResourceRequest& request,
      network::mojom::URLLoaderRequest loader,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      scoped_refptr<net::HttpResponseHeaders> extra_response_headers) {
    // 拥有自己。将与其URLLoader和URLLoaderClientPtr一样生存。
    // 绑定是活动的-本质上直到客户端放弃或全部放弃。
    // 已向其发送文件数据。
    auto* asar_url_loader = new AsarURLLoader;
    asar_url_loader->Start(request, std::move(loader), std::move(client),
                           std::move(extra_response_headers));
  }

  // Network：：mojom：：URLLoader：
  void FollowRedirect(
      const std::vector<std::string>& removed_headers,
      const net::HttpRequestHeaders& modified_headers,
      const net::HttpRequestHeaders& modified_cors_exempt_headers,
      const absl::optional<GURL>& new_url) override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

 private:
  AsarURLLoader() = default;
  ~AsarURLLoader() override = default;

  void Start(const network::ResourceRequest& request,
             mojo::PendingReceiver<network::mojom::URLLoader> loader,
             mojo::PendingRemote<network::mojom::URLLoaderClient> client,
             scoped_refptr<net::HttpResponseHeaders> extra_response_headers) {
    auto head = network::mojom::URLResponseHead::New();
    head->request_start = base::TimeTicks::Now();
    head->response_start = base::TimeTicks::Now();
    head->headers = extra_response_headers;

    base::FilePath path;
    if (!net::FileURLToFilePath(request.url, &path)) {
      mojo::Remote<network::mojom::URLLoaderClient> client_remote(
          std::move(client));
      client_remote->OnComplete(
          network::URLLoaderCompletionStatus(net::ERR_FAILED));
      MaybeDeleteSelf();
      return;
    }

    // 确定它是否为asar文件。
    base::FilePath asar_path, relative_path;
    if (!GetAsarArchivePath(path, &asar_path, &relative_path)) {
      content::CreateFileURLLoaderBypassingSecurityChecks(
          request, std::move(loader), std::move(client), nullptr, false,
          extra_response_headers);
      MaybeDeleteSelf();
      return;
    }

    client_.Bind(std::move(client));
    receiver_.Bind(std::move(loader));
    receiver_.set_disconnect_handler(base::BindOnce(
        &AsarURLLoader::OnConnectionError, base::Unretained(this)));

    // 解析asar存档。
    std::shared_ptr<Archive> archive = GetOrCreateAsarArchive(asar_path);
    Archive::FileInfo info;
    if (!archive || !archive->GetFileInfo(relative_path, &info)) {
      OnClientComplete(net::ERR_FILE_NOT_FOUND);
      return;
    }
    bool is_verifying_file = info.integrity.has_value();

    // 对于解压缩路径，读起来要像普通文件一样。
    base::FilePath real_path;
    if (info.unpacked) {
      archive->CopyFileOut(relative_path, &real_path);
      info.offset = 0;
    }

    mojo::ScopedDataPipeProducerHandle producer_handle;
    mojo::ScopedDataPipeConsumerHandle consumer_handle;
    if (mojo::CreateDataPipe(kDefaultFileUrlPipeSize, producer_handle,
                             consumer_handle) != MOJO_RESULT_OK) {
      OnClientComplete(net::ERR_FAILED);
      return;
    }

    // 请注意，虽然|Archive|已打开|base：：file|，但我们仍需要。
    // 要在此处创建新的|base：：file|，因为它可能被多个。
    // 同时发出请求。
    base::File file(info.unpacked ? real_path : archive->path(),
                    base::File::FLAG_OPEN | base::File::FLAG_READ);
    auto file_data_source =
        std::make_unique<mojo::FileDataSource>(file.Duplicate());
    std::unique_ptr<mojo::DataPipeProducer::DataSource> readable_data_source;
    mojo::FileDataSource* file_data_source_raw = file_data_source.get();
    AsarFileValidator* file_validator_raw = nullptr;
    uint32_t block_size = 0;
    if (info.integrity.has_value()) {
      block_size = info.integrity.value().block_size;
      auto asar_validator = std::make_unique<AsarFileValidator>(
          std::move(info.integrity.value()), std::move(file));
      file_validator_raw = asar_validator.get();
      readable_data_source.reset(new mojo::FilteredDataSource(
          std::move(file_data_source), std::move(asar_validator)));
    } else {
      readable_data_source = std::move(file_data_source);
    }

    std::vector<char> initial_read_buffer(
        std::min(static_cast<uint32_t>(net::kMaxBytesToSniff), info.size));
    auto read_result = readable_data_source.get()->Read(
        info.offset, base::span<char>(initial_read_buffer));
    if (read_result.result != MOJO_RESULT_OK) {
      OnClientComplete(ConvertMojoResultToNetError(read_result.result));
      return;
    }

    std::string range_header;
    net::HttpByteRange byte_range;
    if (request.headers.GetHeader(net::HttpRequestHeaders::kRange,
                                  &range_header)) {
      // 处理单个范围的简单范围标题。
      std::vector<net::HttpByteRange> ranges;
      bool fail = false;
      if (net::HttpUtil::ParseRangeHeader(range_header, &ranges) &&
          ranges.size() == 1) {
        byte_range = ranges[0];
        if (!byte_range.ComputeBounds(info.size))
          fail = true;
      } else {
        fail = true;
      }

      if (fail) {
        OnClientComplete(net::ERR_REQUEST_RANGE_NOT_SATISFIABLE);
        return;
      }
    }

    uint64_t first_byte_to_send = 0;
    uint64_t total_bytes_dropped_from_head = initial_read_buffer.size();
    uint64_t total_bytes_to_send = info.size;

    if (byte_range.IsValid()) {
      first_byte_to_send = byte_range.first_byte_position();
      total_bytes_to_send =
          byte_range.last_byte_position() - first_byte_to_send + 1;
    }

    total_bytes_written_ = total_bytes_to_send;

    head->content_length = base::saturated_cast<int64_t>(total_bytes_to_send);

    if (first_byte_to_send < read_result.bytes_read) {
      // 写入我们读取的用于MIME嗅探的任何数据，并按范围WHERE进行约束。
      // 适用。这将始终适合管道(请参见下面的断言。
      // |kDefaultFileUrlPipeSize|定义)。
      uint32_t write_size = std::min(
          static_cast<uint32_t>(read_result.bytes_read - first_byte_to_send),
          static_cast<uint32_t>(total_bytes_to_send));
      const uint32_t expected_write_size = write_size;
      MojoResult result =
          producer_handle->WriteData(&initial_read_buffer[first_byte_to_send],
                                     &write_size, MOJO_WRITE_DATA_FLAG_NONE);
      if (result != MOJO_RESULT_OK || write_size != expected_write_size) {
        OnFileWritten(result);
        return;
      }

      // 将我们刚刚发送的字节从总范围中打折。
      first_byte_to_send = read_result.bytes_read;
      total_bytes_to_send -= write_size;
    } else if (is_verifying_file &&
               first_byte_to_send >= static_cast<uint64_t>(block_size)) {
      // 如果验证处于活动状态并且请求所需的字节范围开始。
      // 在第一个数据块之后，我们需要读取下一个4MB-1KB以进行验证。
      // 那个街区。然后我们可以跳到SetRange中的目标块。
      // 如果我们遇到这种情况，假设没有读取的数据，请调用下面的。
      // 将是制片人所需要的。
      uint64_t bytes_to_drop = block_size - net::kMaxBytesToSniff;
      total_bytes_dropped_from_head += bytes_to_drop;
      std::vector<char> abandoned_buffer(bytes_to_drop);
      auto abandon_read_result =
          readable_data_source.get()->Read(info.offset + net::kMaxBytesToSniff,
                                           base::span<char>(abandoned_buffer));
      if (abandon_read_result.result != MOJO_RESULT_OK) {
        OnClientComplete(
            ConvertMojoResultToNetError(abandon_read_result.result));
        return;
      }
    }

    if (!net::GetMimeTypeFromFile(path, &head->mime_type)) {
      std::string new_type;
      net::SniffMimeType(
          base::StringPiece(initial_read_buffer.data(), read_result.bytes_read),
          request.url, head->mime_type,
          net::ForceSniffFileUrlsForHtml::kDisabled, &new_type);
      head->mime_type.assign(new_type);
      head->did_mime_sniff = true;
    }
    if (head->headers) {
      head->headers->AddHeader(net::HttpRequestHeaders::kContentType,
                               head->mime_type.c_str());
    }
    client_->OnReceiveResponse(std::move(head));
    client_->OnStartLoadingResponseBody(std::move(consumer_handle));

    if (total_bytes_to_send == 0) {
      // 肯定没有更多的数据了，所以我们已经完成了。
      // 我们将范围数据提供给文件验证器，以便。
      // 它可以验证我们发送的少量数据。
      if (file_validator_raw)
        file_validator_raw->SetRange(info.offset + first_byte_to_send,
                                     total_bytes_dropped_from_head,
                                     info.offset + info.size);
      OnFileWritten(MOJO_RESULT_OK);
      return;
    }

    if (is_verifying_file) {
      int start_block = first_byte_to_send / block_size;

      // 如果我们从第一个街区开始，我们可能不会从。
      // 在那里我们闻了闻。我们可能在一个文件中有几个KB，所以我们需要读取。
      // 将数据放在中间，这样它就会被散列。
      // 
      // 如果我们从较晚的街区出发，我们可能会走到一半。
      // 穿过街区，不管闻到了什么。我们需要读一读。
      // 从初始数据块开始到实际数据块开始的数据。
      // 读取点，这样它就会被散列。
      uint64_t bytes_to_drop =
          start_block == 0 ? first_byte_to_send - net::kMaxBytesToSniff
                           : first_byte_to_send - (start_block * block_size);
      if (file_validator_raw)
        file_validator_raw->SetCurrentBlock(start_block);

      if (bytes_to_drop > 0) {
        uint64_t dropped_bytes_offset =
            info.offset + (start_block * block_size);
        if (start_block == 0)
          dropped_bytes_offset += net::kMaxBytesToSniff;
        total_bytes_dropped_from_head += bytes_to_drop;
        std::vector<char> abandoned_buffer(bytes_to_drop);
        auto abandon_read_result = readable_data_source.get()->Read(
            dropped_bytes_offset, base::span<char>(abandoned_buffer));
        if (abandon_read_result.result != MOJO_RESULT_OK) {
          OnClientComplete(
              ConvertMojoResultToNetError(abandon_read_result.result));
          return;
        }
      }
    }

    // 如果是范围请求，请在以下位置之前找到合适的位置。
    // 异步发送剩余字节。在正常情况下。
    // (即，无范围请求)该寻道实际上是无操作。
    // 
    // 请注意，在Electron中，我们还需要添加文件偏移量。
    file_data_source_raw->SetRange(
        first_byte_to_send + info.offset,
        first_byte_to_send + info.offset + total_bytes_to_send);
    if (file_validator_raw)
      file_validator_raw->SetRange(info.offset + first_byte_to_send,
                                   total_bytes_dropped_from_head,
                                   info.offset + info.size);

    data_producer_ =
        std::make_unique<mojo::DataPipeProducer>(std::move(producer_handle));
    data_producer_->Write(
        std::move(readable_data_source),
        base::BindOnce(&AsarURLLoader::OnFileWritten, base::Unretained(this)));
  }

  void OnConnectionError() {
    receiver_.reset();
    MaybeDeleteSelf();
  }

  void OnClientComplete(net::Error net_error) {
    client_->OnComplete(network::URLLoaderCompletionStatus(net_error));
    client_.reset();
    MaybeDeleteSelf();
  }

  void MaybeDeleteSelf() {
    if (!receiver_.is_bound() && !client_.is_bound())
      delete this;
  }

  void OnFileWritten(MojoResult result) {
    // 现在所有数据都已写入。关闭数据管道。消费者会。
    // 请注意，现在将没有更多数据可供读取。
    data_producer_.reset();

    if (result == MOJO_RESULT_OK) {
      network::URLLoaderCompletionStatus status(net::OK);
      status.encoded_data_length = total_bytes_written_;
      status.encoded_body_length = total_bytes_written_;
      status.decoded_body_length = total_bytes_written_;
      client_->OnComplete(status);
    } else {
      client_->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
    }
    client_.reset();
    MaybeDeleteSelf();
  }

  std::unique_ptr<mojo::DataPipeProducer> data_producer_;
  mojo::Receiver<network::mojom::URLLoader> receiver_{this};
  mojo::Remote<network::mojom::URLLoaderClient> client_;

  // 在加载成功的情况下，它保存写入的总字节数。
  // 响应(在以下情况下，此大小可能小于文件的总大小。
  // 请求了字节范围)。
  // 它用于设置传回的一些URLLoaderCompletionStatus数据。
  // 发送到URLLoaderClients(例如SimpleURLLoader)。
  size_t total_bytes_written_ = 0;

  DISALLOW_COPY_AND_ASSIGN(AsarURLLoader);
};

}  // 命名空间。

void CreateAsarURLLoader(
    const network::ResourceRequest& request,
    network::mojom::URLLoaderRequest loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    scoped_refptr<net::HttpResponseHeaders> extra_response_headers) {
  auto task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&AsarURLLoader::CreateAndStart, request, std::move(loader),
                     std::move(client), std::move(extra_response_headers)));
}

}  // 命名空间asar
