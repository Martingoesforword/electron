// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/web_view_guest_delegate.h"

#include <memory>

#include "content/browser/web_contents/web_contents_impl.h"  // 点名检查。
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "third_party/blink/public/common/page/page_zoom.h"

namespace electron {

WebViewGuestDelegate::WebViewGuestDelegate(content::WebContents* embedder,
                                           api::WebContents* api_web_contents)
    : embedder_web_contents_(embedder), api_web_contents_(api_web_contents) {}

WebViewGuestDelegate::~WebViewGuestDelegate() {
  ResetZoomController();
}

void WebViewGuestDelegate::AttachToIframe(
    content::WebContents* embedder_web_contents,
    int embedder_frame_id) {
  embedder_web_contents_ = embedder_web_contents;

  int embedder_process_id =
      embedder_web_contents_->GetMainFrame()->GetProcess()->GetID();
  auto* embedder_frame =
      content::RenderFrameHost::FromID(embedder_process_id, embedder_frame_id);
  DCHECK_EQ(embedder_web_contents_,
            content::WebContents::FromRenderFrameHost(embedder_frame));

  content::WebContents* guest_web_contents = api_web_contents_->web_contents();

  // 强制刷新WebPreferences，以便OverrideWebkitPrefs在。
  // 在呈现器进程初始化之前的新Web内容。
  // Guest_web_contents-&gt;NotifyPreferencesChanged()；

  // 将此内部WebContents|GUEST_WEB_CONTENTS|附加到外部。
  // WebContents|Embedder_Web_Contents|。外部WebContents的。
  // Frame|Embedder_Frame|托管内部WebContents。
  embedder_web_contents_->AttachInnerWebContents(
      base::WrapUnique<content::WebContents>(guest_web_contents),
      embedder_frame, false);

  ResetZoomController();

  embedder_zoom_controller_ =
      WebContentsZoomController::FromWebContents(embedder_web_contents_);
  embedder_zoom_controller_->AddObserver(this);
  auto* zoom_controller = api_web_contents_->GetZoomController();
  zoom_controller->SetEmbedderZoomController(embedder_zoom_controller_);

  api_web_contents_->Emit("did-attach");
}

void WebViewGuestDelegate::WillDestroy() {
  ResetZoomController();
}

content::WebContents* WebViewGuestDelegate::GetOwnerWebContents() {
  return embedder_web_contents_;
}

void WebViewGuestDelegate::OnZoomLevelChanged(
    content::WebContents* web_contents,
    double level,
    bool is_temporary) {
  if (web_contents == GetOwnerWebContents()) {
    if (is_temporary) {
      api_web_contents_->GetZoomController()->SetTemporaryZoomLevel(level);
    } else {
      api_web_contents_->GetZoomController()->SetZoomLevel(level);
    }
    // 更改默认缩放因子以匹配嵌入器的新缩放级别。
    double zoom_factor = blink::PageZoomFactorToZoomLevel(level);
    api_web_contents_->GetZoomController()->SetDefaultZoomFactor(zoom_factor);
  }
}

void WebViewGuestDelegate::OnZoomControllerWebContentsDestroyed() {
  ResetZoomController();
}

void WebViewGuestDelegate::ResetZoomController() {
  if (embedder_zoom_controller_) {
    embedder_zoom_controller_->RemoveObserver(this);
    embedder_zoom_controller_ = nullptr;
  }
}

content::WebContents* WebViewGuestDelegate::CreateNewGuestWindow(
    const content::WebContents::CreateParams& create_params) {
  // 下面的代码反映了Content：：WebContentsImpl：：CreateNewWindow。
  // 对非来宾来源执行的操作。
  content::WebContents::CreateParams guest_params(create_params);
  guest_params.context = embedder_web_contents_->GetNativeView();
  std::unique_ptr<content::WebContents> guest_contents =
      content::WebContents::Create(guest_params);
  if (!create_params.opener_suppressed) {
    auto* guest_contents_impl =
        static_cast<content::WebContentsImpl*>(guest_contents.release());
    auto* new_guest_view = guest_contents_impl->GetView();
    content::RenderWidgetHostView* widget_view =
        new_guest_view->CreateViewForWidget(
            guest_contents_impl->GetRenderViewHost()->GetWidget());
    if (!create_params.initially_hidden)
      widget_view->Show();
    return guest_contents_impl;
  }
  return guest_contents.release();
}

}  // 命名空间电子
