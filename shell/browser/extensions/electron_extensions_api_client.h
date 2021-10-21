// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_API_CLIENT_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_API_CLIENT_H_

#include <memory>

#include "extensions/browser/api/extensions_api_client.h"

namespace extensions {

class ElectronMessagingDelegate;

class ElectronExtensionsAPIClient : public ExtensionsAPIClient {
 public:
  ElectronExtensionsAPIClient();
  ~ElectronExtensionsAPIClient() override;

  // 扩展APIClient。
  MessagingDelegate* GetMessagingDelegate() override;
  void AttachWebContentsHelpers(
      content::WebContents* web_contents) const override;
  std::unique_ptr<MimeHandlerViewGuestDelegate>
  CreateMimeHandlerViewGuestDelegate(
      MimeHandlerViewGuest* guest) const override;
  ManagementAPIDelegate* CreateManagementAPIDelegate() const override;
  std::unique_ptr<guest_view::GuestViewManagerDelegate>
  CreateGuestViewManagerDelegate(
      content::BrowserContext* context) const override;

 private:
  std::unique_ptr<ElectronMessagingDelegate> messaging_delegate_;
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSIONS_API_CLIENT_H_
