// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/save_page_handler.h"

#include <utility>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/electron_browser_context.h"

namespace electron {

namespace api {

SavePageHandler::SavePageHandler(content::WebContents* web_contents,
                                 gin_helper::Promise<void> promise)
    : web_contents_(web_contents), promise_(std::move(promise)) {}

SavePageHandler::~SavePageHandler() = default;

void SavePageHandler::OnDownloadCreated(content::DownloadManager* manager,
                                        download::DownloadItem* item) {
  // OnDownloadCreated在WebContents：：SavePage期间调用，因此|Item|。
  // 下面是WebContents：：SavePage声明的内容。
  item->AddObserver(this);
}

bool SavePageHandler::Handle(const base::FilePath& full_path,
                             const content::SavePageType& save_type) {
  auto* download_manager =
      web_contents_->GetBrowserContext()->GetDownloadManager();
  download_manager->AddObserver(this);
  // Chrome将在保存目录下创建一个‘foo_files’目录。
  // 页面“foo.html”，用于保存“foo.html”的其他资源文件。
  base::FilePath saved_main_directory_path = full_path.DirName().Append(
      full_path.RemoveExtension().BaseName().value() +
      FILE_PATH_LITERAL("_files"));
  bool result =
      web_contents_->SavePage(full_path, saved_main_directory_path, save_type);
  download_manager->RemoveObserver(this);
  // 如果初始化失败，即无法创建|DownloadItem|，我们需要。
  // 删除|SavePageHandler|实例以避免内存泄漏。
  if (!result) {
    promise_.RejectWithErrorMessage("Failed to save the page");
    delete this;
  }
  return result;
}

void SavePageHandler::OnDownloadUpdated(download::DownloadItem* item) {
  if (item->IsDone()) {
    if (item->GetState() == download::DownloadItem::COMPLETE)
      promise_.Resolve();
    else
      promise_.RejectWithErrorMessage("Failed to save the page.");
    Destroy(item);
  }
}

void SavePageHandler::Destroy(download::DownloadItem* item) {
  item->RemoveObserver(this);
  delete this;
}

}  // 命名空间API。

}  // 命名空间电子
