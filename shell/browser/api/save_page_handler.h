// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_SAVE_PAGE_HANDLER_H_
#define SHELL_BROWSER_API_SAVE_PAGE_HANDLER_H_

#include "components/download/public/common/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/save_page_type.h"
#include "shell/common/gin_helper/promise.h"
#include "v8/include/v8.h"

namespace base {
class FilePath;
}

namespace content {
class WebContents;
}

namespace electron {

namespace api {

// 用于处理保存页请求的自销毁类。
class SavePageHandler : public content::DownloadManager::Observer,
                        public download::DownloadItem::Observer {
 public:
  SavePageHandler(content::WebContents* web_contents,
                  gin_helper::Promise<void> promise);
  ~SavePageHandler() override;

  bool Handle(const base::FilePath& full_path,
              const content::SavePageType& save_type);

 private:
  void Destroy(download::DownloadItem* item);

  // 内容：：下载管理器：：观察者：
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;

  // 下载：：DownloadItem：：观察者：
  void OnDownloadUpdated(download::DownloadItem* item) override;

  content::WebContents* web_contents_;  // 瘦弱。
  gin_helper::Promise<void> promise_;
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_SAVE_PAGE_HANDLER_H_
