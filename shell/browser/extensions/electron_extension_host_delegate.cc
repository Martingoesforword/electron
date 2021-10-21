// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/electron_extension_host_delegate.h"

#include <memory>
#include <string>
#include <utility>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "extensions/browser/media_capture_util.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/extensions/electron_extension_web_contents_observer.h"
#include "v8/include/v8.h"

namespace extensions {

ElectronExtensionHostDelegate::ElectronExtensionHostDelegate() = default;

ElectronExtensionHostDelegate::~ElectronExtensionHostDelegate() = default;

void ElectronExtensionHostDelegate::OnExtensionHostCreated(
    content::WebContents* web_contents) {
  ElectronExtensionWebContentsObserver::CreateForWebContents(web_contents);
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope scope(isolate);
  electron::api::WebContents::FromOrCreate(isolate, web_contents);
}

void ElectronExtensionHostDelegate::OnMainFrameCreatedForBackgroundPage(
    ExtensionHost* host) {}

content::JavaScriptDialogManager*
ElectronExtensionHostDelegate::GetJavaScriptDialogManager() {
  // TODO(JamesCook)：创建一个JavaScriptDialogManager或重用来自。
  // Content_Shell。
  NOTREACHED();
  return nullptr;
}

void ElectronExtensionHostDelegate::CreateTab(
    std::unique_ptr<content::WebContents> web_contents,
    const std::string& extension_id,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture) {
  // TODO(詹姆斯库克)：app_shell应该支持打开弹出窗口吗？
  NOTREACHED();
}

void ElectronExtensionHostDelegate::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback,
    const Extension* extension) {
  // 允许访问麦克风和/或摄像头。
  media_capture_util::GrantMediaStreamRequest(web_contents, request,
                                              std::move(callback), extension);
}

bool ElectronExtensionHostDelegate::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    blink::mojom::MediaStreamType type,
    const Extension* extension) {
  media_capture_util::VerifyMediaAccessPermission(type, extension);
  return true;
}

content::PictureInPictureResult
ElectronExtensionHostDelegate::EnterPictureInPicture(
    content::WebContents* web_contents,
    const viz::SurfaceId& surface_id,
    const gfx::Size& natural_size) {
  NOTREACHED();
  return content::PictureInPictureResult();
}

void ElectronExtensionHostDelegate::ExitPictureInPicture() {
  NOTREACHED();
}

}  // 命名空间扩展
