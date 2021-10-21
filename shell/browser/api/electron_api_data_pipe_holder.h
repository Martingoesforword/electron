// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_DATA_PIPE_HOLDER_H_
#define SHELL_BROWSER_API_ELECTRON_API_DATA_PIPE_HOLDER_H_

#include <string>

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/mojom/data_pipe_getter.mojom.h"

namespace electron {

namespace api {

// 保留对数据管道的引用。
class DataPipeHolder : public gin::Wrappable<DataPipeHolder> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static gin::Handle<DataPipeHolder> Create(
      v8::Isolate* isolate,
      const network::DataElement& element);
  static gin::Handle<DataPipeHolder> From(v8::Isolate* isolate,
                                          const std::string& id);

  // 一次读取所有数据。
  // 
  // TODO(Zcbenz)：这显然不适合非常大的数据，但是。
  // 目前还没有人对此提出抱怨。
  v8::Local<v8::Promise> ReadAll(v8::Isolate* isolate);

  // 可用于接收对象的唯一ID。
  const std::string& id() const { return id_; }

 private:
  explicit DataPipeHolder(const network::DataElement& element);
  ~DataPipeHolder() override;

  std::string id_;
  mojo::Remote<network::mojom::DataPipeGetter> data_pipe_;

  DISALLOW_COPY_AND_ASSIGN(DataPipeHolder);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_DATA_PIPE_HOLDER_H_
