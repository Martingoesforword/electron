// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_WEB_VIEW_GUEST_DELEGATE_H_
#define SHELL_BROWSER_WEB_VIEW_GUEST_DELEGATE_H_

#include "content/public/browser/browser_plugin_guest_delegate.h"
#include "shell/browser/web_contents_zoom_controller.h"

namespace electron {

namespace api {
class WebContents;
}

class WebViewGuestDelegate : public content::BrowserPluginGuestDelegate,
                             public WebContentsZoomController::Observer {
 public:
  WebViewGuestDelegate(content::WebContents* embedder,
                       api::WebContents* api_web_contents);
  ~WebViewGuestDelegate() override;

  // 附着到IFRAME。
  void AttachToIframe(content::WebContents* embedder_web_contents,
                      int embedder_frame_id);
  void WillDestroy();

 protected:
  // 内容：：BrowserPluginGuestDelegate：
  content::WebContents* GetOwnerWebContents() final;
  content::WebContents* CreateNewGuestWindow(
      const content::WebContents::CreateParams& create_params) final;

  // WebContentsZoomController：：观察者：
  void OnZoomLevelChanged(content::WebContents* web_contents,
                          double level,
                          bool is_temporary) override;
  void OnZoomControllerWebContentsDestroyed() override;

 private:
  void ResetZoomController();

  // 附加此来宾视图的WebContents。
  content::WebContents* embedder_web_contents_ = nullptr;

  // 使用的嵌入器的缩放控制器。
  // 若要订阅缩放更改，请执行以下操作。
  WebContentsZoomController* embedder_zoom_controller_ = nullptr;

  api::WebContents* api_web_contents_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WebViewGuestDelegate);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Web_View_Guest_Delegate_H_
