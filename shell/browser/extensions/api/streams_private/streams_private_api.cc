// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "electron/shell/browser/extensions/api/streams_private/streams_private_api.h"

#include <memory>
#include <utility>

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_stream_manager.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "extensions/common/manifest_handlers/mime_types_handler.h"
#include "shell/browser/api/electron_api_web_contents.h"

namespace extensions {

void StreamsPrivateAPI::SendExecuteMimeTypeHandlerEvent(
    const std::string& extension_id,
    const std::string& view_id,
    bool embedded,
    int frame_tree_node_id,
    int render_process_id,
    int render_frame_id,
    blink::mojom::TransferrableURLLoaderPtr transferrable_loader,
    const GURL& original_url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::WebContents* web_contents = nullptr;
  if (frame_tree_node_id != -1) {
    web_contents =
        content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  } else {
    web_contents = content::WebContents::FromRenderFrameHost(
        content::RenderFrameHost::FromID(render_process_id, render_frame_id));
  }
  if (!web_contents)
    return;

  auto* browser_context = web_contents->GetBrowserContext();

  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(browser_context)
          ->enabled_extensions()
          .GetByID(extension_id);
  if (!extension)
    return;

  MimeTypesHandler* handler = MimeTypesHandler::GetHandler(extension);
  if (!handler->HasPlugin())
    return;

  // 如果MIME处理程序使用MimeHandlerViewGuest，则MimeHandlerViewGuest。
  // 将拥有这条流的所有权。
  GURL handler_url(
      extensions::Extension::GetBaseURLFromExtensionId(extension_id).spec() +
      handler->handler_url());
  int tab_id = -1;
  auto* api_contents = electron::api::WebContents::From(web_contents);
  if (api_contents)
    tab_id = api_contents->ID();
  auto stream_container = std::make_unique<extensions::StreamContainer>(
      tab_id, embedded, handler_url, extension_id,
      std::move(transferrable_loader), original_url);
  extensions::MimeHandlerStreamManager::Get(browser_context)
      ->AddStream(view_id, std::move(stream_container), frame_tree_node_id,
                  render_process_id, render_frame_id);
}

}  // 命名空间扩展
