// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_
#define SHELL_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_

#include <string>

#include "base/macros.h"
#include "third_party/blink/public/mojom/loader/transferrable_url_loader.mojom.h"

namespace extensions {

// TODO(Devlin)：它现在只用于MimeTypesHandler API。我们应该。
// 重命名并移动它以明确这一点。Https://crbug.com/890401.。
class StreamsPrivateAPI {
 public:
  // 将onExecuteMimeTypeHandler事件发送到|Extension_id|。如果观众是。
  // 在BrowserPlugin中打开，请指定。
  // 插件。|Embedded|应设置是否嵌入文档。
  // 在另一个文档中。参数|frame_tree_node_id|用于。
  // 顶级插件包。(PDF等)。如果此参数具有有效值。
  // 然后它覆盖|ender_process_id|和|ender_frame_id|参数。
  // |RENDER_PROCESS_ID|是渲染器进程的ID。这个。
  // |render_frame_id|是RenderFrameHost的路由id。
  // 
  // 如果未启用网络服务，则使用|STREAM|，否则为，
  // |TRANSPOABLE_LOADER|和|Original_url|取而代之。
  static void SendExecuteMimeTypeHandlerEvent(
      const std::string& extension_id,
      const std::string& view_id,
      bool embedded,
      int frame_tree_node_id,
      int render_process_id,
      int render_frame_id,
      blink::mojom::TransferrableURLLoaderPtr transferrable_loader,
      const GURL& original_url);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_API_STREAMS_PRIVATE_STREAMS_PRIVATE_API_H_
