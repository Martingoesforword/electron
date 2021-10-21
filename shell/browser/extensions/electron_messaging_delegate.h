// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_MESSAGING_DELEGATE_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_MESSAGING_DELEGATE_H_

#include <memory>
#include <string>

#include "extensions/browser/api/messaging/messaging_delegate.h"

namespace extensions {

// 扩展消息传递API的Chrome特定功能的助手类。
class ElectronMessagingDelegate : public MessagingDelegate {
 public:
  ElectronMessagingDelegate();
  ~ElectronMessagingDelegate() override;

  // MessagingDelegate：
  PolicyPermission IsNativeMessagingHostAllowed(
      content::BrowserContext* browser_context,
      const std::string& native_host_name) override;
  std::unique_ptr<base::DictionaryValue> MaybeGetTabInfo(
      content::WebContents* web_contents) override;
  content::WebContents* GetWebContentsByTabId(
      content::BrowserContext* browser_context,
      int tab_id) override;
  std::unique_ptr<MessagePort> CreateReceiverForTab(
      base::WeakPtr<MessagePort::ChannelDelegate> channel_delegate,
      const std::string& extension_id,
      const PortId& receiver_port_id,
      content::WebContents* receiver_contents,
      int receiver_frame_id) override;
  std::unique_ptr<MessagePort> CreateReceiverForNativeApp(
      content::BrowserContext* browser_context,
      base::WeakPtr<MessagePort::ChannelDelegate> channel_delegate,
      content::RenderFrameHost* source,
      const std::string& extension_id,
      const PortId& receiver_port_id,
      const std::string& native_app_name,
      bool allow_user_level,
      std::string* error_out) override;
  void QueryIncognitoConnectability(
      content::BrowserContext* context,
      const Extension* extension,
      content::WebContents* source_contents,
      const GURL& url,
      base::OnceCallback<void(bool)> callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronMessagingDelegate);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_MESSAGING_DELEGATE_H_
