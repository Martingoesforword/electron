// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/printing/print_render_frame_helper_delegate.h"

#include "content/public/renderer/render_frame.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/constants.h"
#include "extensions/renderer/guest_view/mime_handler_view/post_message_support.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)。

namespace electron {

PrintRenderFrameHelperDelegate::PrintRenderFrameHelperDelegate() = default;

PrintRenderFrameHelperDelegate::~PrintRenderFrameHelperDelegate() = default;

// 如果|FRAME|是进程外PDF扩展，则返回PDF对象元素。
blink::WebElement PrintRenderFrameHelperDelegate::GetPdfElement(
    blink::WebLocalFrame* frame) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  GURL url = frame->GetDocument().Url();
  bool inside_pdf_extension =
      url.SchemeIs(extensions::kExtensionScheme) &&
      url.host_piece() == extension_misc::kPdfExtensionId;
  if (inside_pdf_extension) {
    // Id=“plugin”的&lt;object&gt;创建于。
    // Chrome/browser/resources/pdf/pdf_viewer_base.js.。
    auto viewer_element = frame->GetDocument().GetElementById("viewer");
    if (!viewer_element.IsNull() && !viewer_element.ShadowRoot().IsNull()) {
      auto plugin_element =
          viewer_element.ShadowRoot().QuerySelector("#plugin");
      if (!plugin_element.IsNull()) {
        return plugin_element;
      }
    }
    NOTREACHED();
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)。
  return blink::WebElement();
}

bool PrintRenderFrameHelperDelegate::IsPrintPreviewEnabled() {
  return false;
}

bool PrintRenderFrameHelperDelegate::OverridePrint(
    blink::WebLocalFrame* frame) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  auto* post_message_support =
      extensions::PostMessageSupport::FromWebLocalFrame(frame);
  if (post_message_support) {
    // 此消息在Chrome/Browser/Resources/pdf/pdf_viewer.js和。
    // 指示打印PDF插件。这将使window.print()在。
    // PDF插件文档可正确打印PDF。看见。
    // Https://crbug.com/448720.。
    base::DictionaryValue message;
    message.SetString("type", "print");
    post_message_support->PostMessageFromValue(message);
    return true;
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)。
  return false;
}

}  // 命名空间电子
