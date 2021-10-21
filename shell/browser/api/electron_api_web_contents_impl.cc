// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_web_contents.h"

#include "content/browser/renderer_host/frame_tree.h"        // 点名检查。
#include "content/browser/renderer_host/frame_tree_node.h"   // 点名检查。
#include "content/browser/web_contents/web_contents_impl.h"  // 点名检查。

#if BUILDFLAG(ENABLE_OSR)
#include "shell/browser/osr/osr_render_widget_host_view.h"
#include "shell/browser/osr/osr_web_contents_view.h"
#endif

// 同时包含web_content_impl.h和node.h会导致错误，我们。
// 我必须将WebContentsImpl的用法隔离到一个干净的文件中才能修复它：
// 错误C2371：‘ssize_t’：重定义；不同的基本类型。

namespace electron {

namespace api {

void WebContents::DetachFromOuterFrame() {
  // 有关如何分离的信息，请参见Detach_webview_frame.patch。
  int frame_tree_node_id =
      static_cast<content::WebContentsImpl*>(web_contents())
          ->GetOuterDelegateFrameTreeNodeId();
  if (frame_tree_node_id != content::FrameTreeNode::kFrameTreeNodeInvalidId) {
    auto* node = content::FrameTreeNode::GloballyFindByID(frame_tree_node_id);
    DCHECK(node->parent());
    node->frame_tree()->RemoveFrame(node);
  }
}

#if BUILDFLAG(ENABLE_OSR)
OffScreenWebContentsView* WebContents::GetOffScreenWebContentsView() const {
  if (IsOffScreen()) {
    const auto* impl =
        static_cast<const content::WebContentsImpl*>(web_contents());
    return static_cast<OffScreenWebContentsView*>(impl->GetView());
  } else {
    return nullptr;
  }
}

OffScreenRenderWidgetHostView* WebContents::GetOffScreenRenderWidgetHostView()
    const {
  if (IsOffScreen()) {
    return static_cast<OffScreenRenderWidgetHostView*>(
        web_contents()->GetRenderWidgetHostView());
  } else {
    return nullptr;
  }
}
#endif

}  // 命名空间API。

}  // 命名空间电子
