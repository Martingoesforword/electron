// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_
#define SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/download_manager_delegate.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/common/gin_helper/dictionary.h"

namespace content {
class DownloadManager;
}

namespace electron {

class ElectronDownloadManagerDelegate
    : public content::DownloadManagerDelegate {
 public:
  using CreateDownloadPathCallback =
      base::RepeatingCallback<void(const base::FilePath&)>;

  explicit ElectronDownloadManagerDelegate(content::DownloadManager* manager);
  ~ElectronDownloadManagerDelegate() override;

  // 内容：：DownloadManagerDelegate：
  void Shutdown() override;
  bool DetermineDownloadTarget(
      download::DownloadItem* download,
      content::DownloadTargetCallback* callback) override;
  bool ShouldOpenDownload(
      download::DownloadItem* download,
      content::DownloadOpenDelayedCallback callback) override;
  void GetNextId(content::DownloadIdCallback callback) override;

 private:
  // 获取在关联的API：：DownloadItem对象上设置的保存路径。
  void GetItemSavePath(download::DownloadItem* item, base::FilePath* path);
  void GetItemSaveDialogOptions(download::DownloadItem* item,
                                file_dialog::DialogSettings* options);

  void OnDownloadPathGenerated(uint32_t download_id,
                               content::DownloadTargetCallback callback,
                               const base::FilePath& default_path);

  void OnDownloadSaveDialogDone(
      uint32_t download_id,
      content::DownloadTargetCallback download_callback,
      gin_helper::Dictionary result);

  base::FilePath last_saved_directory_;

  content::DownloadManager* download_manager_;
  base::WeakPtrFactory<ElectronDownloadManagerDelegate> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ElectronDownloadManagerDelegate);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_ELECTRON_DOWNLOAD_MANAGER_DELEGATE_H_
