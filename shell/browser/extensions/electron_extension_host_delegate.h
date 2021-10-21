// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_HOST_DELEGATE_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_HOST_DELEGATE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "extensions/browser/extension_host_delegate.h"

namespace extensions {

// 最小扩展HostDelegate。
class ElectronExtensionHostDelegate : public ExtensionHostDelegate {
 public:
  ElectronExtensionHostDelegate();
  ~ElectronExtensionHostDelegate() override;

  // ExtensionHostDelegate实现。
  void OnExtensionHostCreated(content::WebContents* web_contents) override;
  void OnMainFrameCreatedForBackgroundPage(ExtensionHost* host) override;
  content::JavaScriptDialogManager* GetJavaScriptDialogManager() override;
  void CreateTab(std::unique_ptr<content::WebContents> web_contents,
                 const std::string& extension_id,
                 WindowOpenDisposition disposition,
                 const gfx::Rect& initial_rect,
                 bool user_gesture) override;
  void ProcessMediaAccessRequest(content::WebContents* web_contents,
                                 const content::MediaStreamRequest& request,
                                 content::MediaResponseCallback callback,
                                 const Extension* extension) override;
  bool CheckMediaAccessPermission(content::RenderFrameHost* render_frame_host,
                                  const GURL& security_origin,
                                  blink::mojom::MediaStreamType type,
                                  const Extension* extension) override;
  content::PictureInPictureResult EnterPictureInPicture(
      content::WebContents* web_contents,
      const viz::SurfaceId& surface_id,
      const gfx::Size& natural_size) override;
  void ExitPictureInPicture() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionHostDelegate);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_HOST_DELEGATE_H_
