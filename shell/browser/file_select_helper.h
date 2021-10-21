// 版权所有(C)2021 Microsoft。版权所有。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_FILE_SELECT_HELPER_H_
#define SHELL_BROWSER_FILE_SELECT_HELPER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/scoped_observation.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "net/base/directory_lister.h"
#include "third_party/blink/public/mojom/choosers/file_chooser.mojom.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace content {
class FileSelectListener;
class WebContents;
}  // 命名空间内容。

namespace ui {
struct SelectedFileInfo;
}

// 此类处理来自渲染器进程的文件选择请求。
// 它实现了的初始化和监听器函数。
// 文件选择对话框。
// 
// 由于FileSelectHelper监听对小部件的观察，因此它需要存活。
// 并在UI线程上销毁。对FileSelectHelper的引用可能是。
// 传递给其他线程。
class FileSelectHelper : public base::RefCountedThreadSafe<
                             FileSelectHelper,
                             content::BrowserThread::DeleteOnUIThread>,
                         public ui::SelectFileDialog::Listener,
                         public content::WebContentsObserver,
                         public content::RenderWidgetHostObserver,
                         private net::DirectoryLister::DirectoryListerDelegate {
 public:
  // 显示文件选择器对话框。
  static void RunFileChooser(
      content::RenderFrameHost* render_frame_host,
      scoped_refptr<content::FileSelectListener> listener,
      const blink::mojom::FileChooserParams& params);

  // 枚举目录中的所有文件。
  static void EnumerateDirectory(
      content::WebContents* tab,
      scoped_refptr<content::FileSelectListener> listener,
      const base::FilePath& path);

 private:
  friend class base::RefCountedThreadSafe<FileSelectHelper>;
  friend class base::DeleteHelper<FileSelectHelper>;
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;

  FileSelectHelper();
  ~FileSelectHelper() override;

  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      scoped_refptr<content::FileSelectListener> listener,
                      blink::mojom::FileChooserParamsPtr params);
  void GetFileTypesInThreadPool(blink::mojom::FileChooserParamsPtr params);
  void GetSanitizedFilenameOnUIThread(
      blink::mojom::FileChooserParamsPtr params);

  void RunFileChooserOnUIThread(const base::FilePath& default_path,
                                blink::mojom::FileChooserParamsPtr params);

  // 清理并释放此实例。这必须在最后一个之后调用。
  // 从文件选择器对话框接收回调。
  void RunFileChooserEnd();

  // SelectFileDialog：：Listener覆盖。
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;
  void FileSelectedWithExtraInfo(const ui::SelectedFileInfo& file,
                                 int index,
                                 void* params) override;
  void MultiFilesSelected(const std::vector<base::FilePath>& files,
                          void* params) override;
  void MultiFilesSelectedWithExtraInfo(
      const std::vector<ui::SelectedFileInfo>& files,
      void* params) override;
  void FileSelectionCanceled(void* params) override;

  // 内容：：RenderWidgetHostWatch覆盖。
  void RenderWidgetHostDestroyed(
      content::RenderWidgetHost* widget_host) override;

  // 内容：：WebContentsViewer重写。
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void WebContentsDestroyed() override;

  void EnumerateDirectoryImpl(
      content::WebContents* tab,
      scoped_refptr<content::FileSelectListener> listener,
      const base::FilePath& path);

  // 开始新的目录枚举。
  void StartNewEnumeration(const base::FilePath& path);

  // NET：：DirectoryLister：：DirectoryListerDelegate重写。
  void OnListFile(
      const net::DirectoryLister::DirectoryListerData& data) override;
  void OnListDone(int error) override;

  void LaunchConfirmationDialog(
      const base::FilePath& path,
      std::vector<ui::SelectedFileInfo> selected_files);

  // 清理并释放此实例。这必须在最后一个之后调用。
  // 从枚举代码接收回调。
  void EnumerateDirectoryEnd();

#if defined(OS_MAC)
  // 必须从MayBlock()任务调用。作为软件包的每个选定文件。
  // 将被压缩，并且该压缩文件将被传递到适当的渲染视图主机。
  // 包裹的一部分。
  void ProcessSelectedFilesMac(const std::vector<ui::SelectedFileInfo>& files);

  // 保存|zipping_files|的路径以供以后删除。将|文件|传递给。
  // 渲染视图主机。
  void ProcessSelectedFilesMacOnUIThread(
      const std::vector<ui::SelectedFileInfo>& files,
      const std::vector<base::FilePath>& zipped_files);

  // 将位于|PATH|的包压缩到临时目的地。返回。
  // 如果压缩成功，则返回临时目的地。否则返回一个。
  // 空路径。
  static base::FilePath ZipPackage(const base::FilePath& path);
#endif  // 已定义(OS_MAC)。

  void ConvertToFileChooserFileInfoList(
      const std::vector<ui::SelectedFileInfo>& files);

  // 检查是否需要扫描指定的文件。
  void PerformContentAnalysisIfNeeded(
      std::vector<blink::mojom::FileChooserFileInfoPtr> list);

  // 完成PerformContentAnalysisIfNeeded()处理。
  // 已执行深度扫描检查。深度扫描可能会改变。
  // 用户选择的文件列表，因此此处传递的文件列表可能是。
  // 传递给PerformContentAnalysisIfNeeded()的文件的子集。
  void NotifyListenerAndEnd(
      std::vector<blink::mojom::FileChooserFileInfoPtr> list);

  // 计划删除|TEMPORARY_FILES_|中的文件并清除。
  // 矢量。
  void DeleteTemporaryFiles();

  // 当文件选择器的启动器不再有效时进行清理。
  void CleanUp();

  // 如果Web内容已销毁，则调用RunFileChooserEnd()。返回TRUE。
  // 如果文件选择器操作不应继续。
  bool AbortIfWebContentsDestroyed();

  void SetFileSelectListenerForTesting(
      scoped_refptr<content::FileSelectListener> listener);

  // 用于获取选择文件对话框允许的扩展名的帮助器方法。
  // 规范中定义的指定接受类型：
  // Http://whatwg.org/html/number-state.html#attr-input-accept。
  // |Accept_Types|仅包含有效的小写MIME类型或文件扩展名。
  // 以句点(.)开头。
  static std::unique_ptr<ui::SelectFileDialog::FileTypeInfo>
  GetFileTypesFromAcceptType(const std::vector<std::u16string>& accept_types);

  // 检查接受类型是否有效。它应该都是小写的，
  // 没有空格。
  static bool IsAcceptTypeValid(const std::string& accept_type);

  // 获取适合用作默认文件名的已清理文件名。
  static base::FilePath GetSanitizedFileName(
      const base::FilePath& suggested_path);

  // 显示文件对话框的页面的RenderFrameHost和WebContents。
  // (可能只有一个这样的对话框)。
  content::RenderFrameHost* render_frame_host_;
  content::WebContents* web_contents_;

  // |listener_|接收FileSelectHelper的结果。
  scoped_refptr<content::FileSelectListener> listener_;

  // 用于从文件表单域选择要上载的文件的对话框。
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> select_file_types_;

  // 上次显示的文件对话框类型。这是SELECT_NONE，如果。
  // 实例是通过公共EnumerateDirectory()创建的。
  ui::SelectFileDialog::Type dialog_type_;

  // 上次显示的文件对话框模式。
  blink::mojom::FileChooserParams::Mode dialog_mode_;

  // EnumerateDirectory()和EnumerateDirectory()的枚举根目录。
  // 使用kUploadFolder运行FileChooser。
  base::FilePath base_dir_;

  // 维护Active Directory枚举。这些可能来自文件。
  // 选择对话框或从拖放的目录中选择。不可能有。
  // 一次不止一次。
  struct ActiveDirectoryEnumeration;
  std::unique_ptr<ActiveDirectoryEnumeration> directory_enumeration_;

  base::ScopedObservation<content::RenderWidgetHost,
                          content::RenderWidgetHostObserver>
      observation_{this};

  // 仅在OSX上使用的临时文件。这个类负责删除。
  // 当不再需要这些文件时，请执行以下操作。
  std::vector<base::FilePath> temporary_files_;

  DISALLOW_COPY_AND_ASSIGN(FileSelectHelper);
};

#endif  // Shell_Browser_File_Select_Helper_H_
