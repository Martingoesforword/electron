// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_WEB_VIEW_MANAGER_H_
#define SHELL_BROWSER_WEB_VIEW_MANAGER_H_

#include <map>

#include "content/public/browser/browser_plugin_guest_manager.h"

namespace electron {

class WebViewManager : public content::BrowserPluginGuestManager {
 public:
  WebViewManager();
  ~WebViewManager() override;

  void AddGuest(int guest_instance_id,
                content::WebContents* embedder,
                content::WebContents* web_contents);
  void RemoveGuest(int guest_instance_id);

  static WebViewManager* GetWebViewManager(content::WebContents* web_contents);

  // 内容：：BrowserPluginGuestManager：
  bool ForEachGuest(content::WebContents* embedder,
                    const GuestCallback& callback) override;

 private:
  struct WebContentsWithEmbedder {
    content::WebContents* web_contents;
    content::WebContents* embedder;
  };
  // Guest_instance_id=&gt;(web_content，嵌入器)。
  std::map<int, WebContentsWithEmbedder> web_contents_embedder_map_;

  DISALLOW_COPY_AND_ASSIGN(WebViewManager);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Web_VIEW_MANAGER_H_
