// 版权所有2017年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/electron_messaging_delegate.h"

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/api/messaging/extension_message_port.h"
#include "extensions/browser/api/messaging/native_message_host.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/api/messaging/port_id.h"
#include "extensions/common/extension.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "ui/gfx/native_widget_types.h"
#include "url/gurl.h"

namespace extensions {

ElectronMessagingDelegate::ElectronMessagingDelegate() = default;
ElectronMessagingDelegate::~ElectronMessagingDelegate() = default;

MessagingDelegate::PolicyPermission
ElectronMessagingDelegate::IsNativeMessagingHostAllowed(
    content::BrowserContext* browser_context,
    const std::string& native_host_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  return PolicyPermission::DISALLOW;
}

std::unique_ptr<base::DictionaryValue>
ElectronMessagingDelegate::MaybeGetTabInfo(content::WebContents* web_contents) {
  if (web_contents) {
    auto* api_contents = electron::api::WebContents::From(web_contents);
    if (api_contents) {
      auto tab = std::make_unique<base::DictionaryValue>();
      tab->SetWithoutPathExpansion(
          "id", std::make_unique<base::Value>(api_contents->ID()));
      tab->SetWithoutPathExpansion(
          "url", std::make_unique<base::Value>(api_contents->GetURL().spec()));
      tab->SetWithoutPathExpansion(
          "title", std::make_unique<base::Value>(api_contents->GetTitle()));
      tab->SetWithoutPathExpansion(
          "audible",
          std::make_unique<base::Value>(api_contents->IsCurrentlyAudible()));
      return tab;
    }
  }
  return nullptr;
}

content::WebContents* ElectronMessagingDelegate::GetWebContentsByTabId(
    content::BrowserContext* browser_context,
    int tab_id) {
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents) {
    return nullptr;
  }
  return contents->web_contents();
}

std::unique_ptr<MessagePort> ElectronMessagingDelegate::CreateReceiverForTab(
    base::WeakPtr<MessagePort::ChannelDelegate> channel_delegate,
    const std::string& extension_id,
    const PortId& receiver_port_id,
    content::WebContents* receiver_contents,
    int receiver_frame_id) {
  // 帧ID-1是选项卡中的每个帧。
  bool include_child_frames = receiver_frame_id == -1;
  content::RenderFrameHost* receiver_rfh =
      include_child_frames ? receiver_contents->GetMainFrame()
                           : ExtensionApiFrameIdMap::GetRenderFrameHostById(
                                 receiver_contents, receiver_frame_id);
  if (!receiver_rfh)
    return nullptr;

  return std::make_unique<ExtensionMessagePort>(
      channel_delegate, receiver_port_id, extension_id, receiver_rfh,
      include_child_frames);
}

std::unique_ptr<MessagePort>
ElectronMessagingDelegate::CreateReceiverForNativeApp(
    content::BrowserContext* browser_context,
    base::WeakPtr<MessagePort::ChannelDelegate> channel_delegate,
    content::RenderFrameHost* source,
    const std::string& extension_id,
    const PortId& receiver_port_id,
    const std::string& native_app_name,
    bool allow_user_level,
    std::string* error_out) {
  return nullptr;
}

void ElectronMessagingDelegate::QueryIncognitoConnectability(
    content::BrowserContext* context,
    const Extension* target_extension,
    content::WebContents* source_contents,
    const GURL& source_url,
    base::OnceCallback<void(bool)> callback) {
  DCHECK(context->IsOffTheRecord());
  std::move(callback).Run(false);
}

}  // 命名空间扩展
