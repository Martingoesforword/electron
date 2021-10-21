// 版权所有(C)2021 Microsoft。版权所有。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/file_select_helper.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/threading/hang_watcher.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/native_window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/shell_dialogs/selected_file_info.h"

using blink::mojom::FileChooserFileInfo;
using blink::mojom::FileChooserFileInfoPtr;
using blink::mojom::FileChooserParams;
using blink::mojom::FileChooserParamsPtr;
using content::BrowserThread;
using content::RenderViewHost;
using content::RenderWidgetHost;
using content::WebContents;

namespace {

void DeleteFiles(std::vector<base::FilePath> paths) {
  for (auto& file_path : paths)
    base::DeleteFile(file_path);
}

}  // 命名空间。

struct FileSelectHelper::ActiveDirectoryEnumeration {
  explicit ActiveDirectoryEnumeration(const base::FilePath& path)
      : path_(path) {}

  std::unique_ptr<net::DirectoryLister> lister_;
  const base::FilePath path_;
  std::vector<base::FilePath> results_;
};

FileSelectHelper::FileSelectHelper()
    : render_frame_host_(nullptr),
      web_contents_(nullptr),
      select_file_dialog_(),
      select_file_types_(),
      dialog_type_(ui::SelectFileDialog::SELECT_OPEN_FILE),
      dialog_mode_(FileChooserParams::Mode::kOpen) {}

FileSelectHelper::~FileSelectHelper() {
  // 可能有挂起的文件对话框，我们需要告诉他们我们已离开。
  // 这样他们就不会试图回电给我们了。
  if (select_file_dialog_.get())
    select_file_dialog_->ListenerDestroyed();
}

void FileSelectHelper::FileSelected(const base::FilePath& path,
                                    int index,
                                    void* params) {
  FileSelectedWithExtraInfo(ui::SelectedFileInfo(path, path), index, params);
}

void FileSelectHelper::FileSelectedWithExtraInfo(
    const ui::SelectedFileInfo& file,
    int index,
    void* params) {
  if (!render_frame_host_) {
    RunFileChooserEnd();
    return;
  }

  const base::FilePath& path = file.local_path;
  if (dialog_type_ == ui::SelectFileDialog::SELECT_UPLOAD_FOLDER) {
    StartNewEnumeration(path);
    return;
  }

  std::vector<ui::SelectedFileInfo> files;
  files.push_back(file);

#if defined(OS_MAC)
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&FileSelectHelper::ProcessSelectedFilesMac, this, files));
#else
  ConvertToFileChooserFileInfoList(files);
#endif  // 已定义(OS_MAC)。
}

void FileSelectHelper::MultiFilesSelected(
    const std::vector<base::FilePath>& files,
    void* params) {
  std::vector<ui::SelectedFileInfo> selected_files =
      ui::FilePathListToSelectedFileInfoList(files);

  MultiFilesSelectedWithExtraInfo(selected_files, params);
}

void FileSelectHelper::MultiFilesSelectedWithExtraInfo(
    const std::vector<ui::SelectedFileInfo>& files,
    void* params) {
#if defined(OS_MAC)
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&FileSelectHelper::ProcessSelectedFilesMac, this, files));
#else
  ConvertToFileChooserFileInfoList(files);
#endif  // 已定义(OS_MAC)。
}

void FileSelectHelper::FileSelectionCanceled(void* params) {
  RunFileChooserEnd();
}

void FileSelectHelper::StartNewEnumeration(const base::FilePath& path) {
  base_dir_ = path;
  auto entry = std::make_unique<ActiveDirectoryEnumeration>(path);
  entry->lister_ = base::WrapUnique(new net::DirectoryLister(
      path, net::DirectoryLister::NO_SORT_RECURSIVE, this));
  entry->lister_->Start();
  directory_enumeration_ = std::move(entry);
}

void FileSelectHelper::OnListFile(
    const net::DirectoryLister::DirectoryListerData& data) {
  // 目录上传只关心文件。
  if (data.info.IsDirectory())
    return;

  directory_enumeration_->results_.push_back(data.path);
}

void FileSelectHelper::LaunchConfirmationDialog(
    const base::FilePath& path,
    std::vector<ui::SelectedFileInfo> selected_files) {
  ShowFolderUploadConfirmationDialog(
      path,
      base::BindOnce(&FileSelectHelper::ConvertToFileChooserFileInfoList, this),
      std::move(selected_files), web_contents_);
}

void FileSelectHelper::OnListDone(int error) {
  if (!web_contents_) {
    // 在我们的控制下，Web内容已被销毁(可能是通过关闭选项卡)。我们。
    // 必须通知|Listener_|并释放对。
    // 我们自己。RunFileChooserEnd()执行此操作。
    RunFileChooserEnd();
    return;
  }

  // 完成此功能后，需要清除此条目。
  std::unique_ptr<ActiveDirectoryEnumeration> entry =
      std::move(directory_enumeration_);
  if (error) {
    FileSelectionCanceled(NULL);
    return;
  }

  std::vector<ui::SelectedFileInfo> selected_files =
      ui::FilePathListToSelectedFileInfoList(entry->results_);

  if (dialog_type_ == ui::SelectFileDialog::SELECT_UPLOAD_FOLDER) {
    LaunchConfirmationDialog(entry->path_, std::move(selected_files));
  } else {
    std::vector<FileChooserFileInfoPtr> chooser_files;
    for (const auto& file_path : entry->results_) {
      chooser_files.push_back(FileChooserFileInfo::NewNativeFile(
          blink::mojom::NativeFileInfo::New(file_path, std::u16string())));
    }

    listener_->FileSelected(std::move(chooser_files), base_dir_,
                            FileChooserParams::Mode::kUploadFolder);
    listener_.reset();
    EnumerateDirectoryEnd();
  }
}

void FileSelectHelper::ConvertToFileChooserFileInfoList(
    const std::vector<ui::SelectedFileInfo>& files) {
  if (AbortIfWebContentsDestroyed())
    return;

  std::vector<FileChooserFileInfoPtr> chooser_files;
  for (const auto& file : files) {
    chooser_files.push_back(
        FileChooserFileInfo::NewNativeFile(blink::mojom::NativeFileInfo::New(
            file.local_path,
            base::FilePath(file.display_name).AsUTF16Unsafe())));
  }

  PerformContentAnalysisIfNeeded(std::move(chooser_files));
}

void FileSelectHelper::PerformContentAnalysisIfNeeded(
    std::vector<FileChooserFileInfoPtr> list) {
  if (AbortIfWebContentsDestroyed())
    return;

  NotifyListenerAndEnd(std::move(list));
}

void FileSelectHelper::NotifyListenerAndEnd(
    std::vector<blink::mojom::FileChooserFileInfoPtr> list) {
  listener_->FileSelected(std::move(list), base_dir_, dialog_mode_);
  listener_.reset();

  // 从现在开始不应访问任何成员。
  RunFileChooserEnd();
}

void FileSelectHelper::DeleteTemporaryFiles() {
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&DeleteFiles, std::move(temporary_files_)));
}

void FileSelectHelper::CleanUp() {
  if (!temporary_files_.empty()) {
    DeleteTemporaryFiles();

    // 现在，临时文件已被安排删除，
    // 不再有任何理由保留这个实例。
    Release();
  }
}

bool FileSelectHelper::AbortIfWebContentsDestroyed() {
  if (render_frame_host_ == nullptr || web_contents_ == nullptr) {
    RunFileChooserEnd();
    return true;
  }

  return false;
}

void FileSelectHelper::SetFileSelectListenerForTesting(
    scoped_refptr<content::FileSelectListener> listener) {
  DCHECK(listener);
  DCHECK(!listener_);
  listener_ = std::move(listener);
}

std::unique_ptr<ui::SelectFileDialog::FileTypeInfo>
FileSelectHelper::GetFileTypesFromAcceptType(
    const std::vector<std::u16string>& accept_types) {
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> base_file_type(
      new ui::SelectFileDialog::FileTypeInfo());
  if (accept_types.empty())
    return base_file_type;

  // 创建FileTypeInfo并为第一个扩展名列表预分配。
  std::unique_ptr<ui::SelectFileDialog::FileTypeInfo> file_type(
      new ui::SelectFileDialog::FileTypeInfo(*base_file_type));
  file_type->include_all_files = true;
  file_type->extensions.resize(1);
  std::vector<base::FilePath::StringType>* extensions =
      &file_type->extensions.back();

  // 查找相应的分机。
  int valid_type_count = 0;
  int description_id = 0;
  for (const auto& accept_type : accept_types) {
    size_t old_extension_size = extensions->size();
    if (accept_type[0] == '.') {
      // 如果类型以句点开头，则假定为文件扩展名。
      // 所以我们只要把它加到清单上就行了。
      base::FilePath::StringType ext =
          base::FilePath::FromUTF16Unsafe(accept_type).value();
      extensions->push_back(ext.substr(1));
    } else {
      if (!base::IsStringASCII(accept_type))
        continue;
      std::string ascii_type = base::UTF16ToASCII(accept_type);
      if (ascii_type == "image/*")
        description_id = IDS_IMAGE_FILES;
      else if (ascii_type == "audio/*")
        description_id = IDS_AUDIO_FILES;
      else if (ascii_type == "video/*")
        description_id = IDS_VIDEO_FILES;

      net::GetExtensionsForMimeType(ascii_type, extensions);
    }

    if (extensions->size() > old_extension_size)
      valid_type_count++;
  }

  // 如果没有添加有效的延期，则退出。
  if (valid_type_count == 0)
    return base_file_type;

  // 如果符合以下任一条件，请使用通用描述“自定义文件”
  // 正确：
  // 1)指定多种类型，如“音频/*、视频/*”
  // 2)没有参数的MIME类型有多个扩展，例如。
  // “ehtml，shtml，htm，html”代表“text/html”。在Windows上，选择的文件。
  // 对话框使用列表中的第一个扩展名来形成描述，
  // 比如“EHTML文件”。这不是我们想要的。
  if (valid_type_count > 1 ||
      (valid_type_count == 1 && description_id == 0 && extensions->size() > 1))
    description_id = IDS_CUSTOM_FILES;

  if (description_id) {
    file_type->extension_description_overrides.push_back(
        l10n_util::GetStringUTF16(description_id));
  }

  return file_type;
}

// 静电。
void FileSelectHelper::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const FileChooserParams& params) {
  // FileSelectHelper将使自身保持活动状态，直到它发送结果。
  // 留言。
  scoped_refptr<FileSelectHelper> file_select_helper(new FileSelectHelper());
  file_select_helper->RunFileChooser(render_frame_host, std::move(listener),
                                     params.Clone());
}

// 静电。
void FileSelectHelper::EnumerateDirectory(
    content::WebContents* tab,
    scoped_refptr<content::FileSelectListener> listener,
    const base::FilePath& path) {
  // FileSelectHelper将使自身保持活动状态，直到它发送结果。
  // 留言。
  scoped_refptr<FileSelectHelper> file_select_helper(new FileSelectHelper());
  file_select_helper->EnumerateDirectoryImpl(tab, std::move(listener), path);
}

void FileSelectHelper::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    FileChooserParamsPtr params) {
  DCHECK(!render_frame_host_);
  DCHECK(!web_contents_);
  DCHECK(listener);
  DCHECK(!listener_);
  DCHECK(params->default_file_name.empty() ||
         params->mode == FileChooserParams::Mode::kSave)
      << "The default_file_name parameter should only be specified for Save "
         "file choosers";
  DCHECK(params->default_file_name == params->default_file_name.BaseName())
      << "The default_file_name parameter should not contain path separators";

  render_frame_host_ = render_frame_host;
  web_contents_ = WebContents::FromRenderFrameHost(render_frame_host);
  listener_ = std::move(listener);
  observation_.Reset();
  content::WebContentsObserver::Observe(web_contents_);
  observation_.Observe(render_frame_host_->GetRenderViewHost()->GetWidget());

  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&FileSelectHelper::GetFileTypesInThreadPool, this,
                     std::move(params)));

  // 因为该类将通知返回给RenderViewHost，所以它。
  // 呼叫者很难知道这个引用要保留多长时间。
  // 举个例子。我们在这里添加Ref()，以便在返回后保持实例处于活动状态。
  // 直到从文件对话框接收到最后一个回调。
  // 此时，我们必须调用RunFileChooserEnd()。
  AddRef();
}

void FileSelectHelper::GetFileTypesInThreadPool(FileChooserParamsPtr params) {
  select_file_types_ = GetFileTypesFromAcceptType(params->accept_types);
  select_file_types_->allowed_paths =
      params->need_local_path ? ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH
                              : ui::SelectFileDialog::FileTypeInfo::ANY_PATH;

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&FileSelectHelper::GetSanitizedFilenameOnUIThread, this,
                     std::move(params)));
}

void FileSelectHelper::GetSanitizedFilenameOnUIThread(
    FileChooserParamsPtr params) {
  if (AbortIfWebContentsDestroyed())
    return;

  auto* browser_context = static_cast<electron::ElectronBrowserContext*>(
      render_frame_host_->GetProcess()->GetBrowserContext());
  base::FilePath default_file_path =
      browser_context->prefs()
          ->GetFilePath(prefs::kSelectFileLastDirectory)
          .Append(params->default_file_name);

  RunFileChooserOnUIThread(default_file_path, std::move(params));
}

void FileSelectHelper::RunFileChooserOnUIThread(
    const base::FilePath& default_file_path,
    FileChooserParamsPtr params) {
  DCHECK(params);

  select_file_dialog_ = ui::SelectFileDialog::Create(this, nullptr);
  if (!select_file_dialog_.get())
    return;

  dialog_mode_ = params->mode;
  switch (params->mode) {
    case FileChooserParams::Mode::kOpen:
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_FILE;
      break;
    case FileChooserParams::Mode::kOpenMultiple:
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE;
      break;
    case FileChooserParams::Mode::kUploadFolder:
      dialog_type_ = ui::SelectFileDialog::SELECT_UPLOAD_FOLDER;
      break;
    case FileChooserParams::Mode::kSave:
      dialog_type_ = ui::SelectFileDialog::SELECT_SAVEAS_FILE;
      break;
    default:
      // 防止警告。
      dialog_type_ = ui::SelectFileDialog::SELECT_OPEN_FILE;
      NOTREACHED();
  }

  auto* web_contents = electron::api::WebContents::From(
      content::WebContents::FromRenderFrameHost(render_frame_host_));
  if (!web_contents || !web_contents->owner_window())
    return;

  // 切勿将当前作用域视为挂起。挂起观看截止日期(如果。
  // 任何)是无效的，因为用户可能会花费无限的时间来选择。
  // 档案。
  base::HangWatcher::InvalidateActiveExpectations();

  select_file_dialog_->SelectFile(
      dialog_type_, params->title, default_file_path, select_file_types_.get(),
      select_file_types_.get() && !select_file_types_->extensions.empty()
          ? 1
          : 0,  // 要显示的默认扩展名的从1开始的索引。
      base::FilePath::StringType(),
      web_contents->owner_window()->GetNativeWindow(), NULL);

  select_file_types_.reset();
}

// 当我们收到来自文件选择器的最后一个回调时，将调用此方法。
// 对话框，或者渲染器是否被销毁。执行任何清理并释放。
// 我们在RunFileChooser()中添加的引用。
void FileSelectHelper::RunFileChooserEnd() {
  // 如果有临时文件，则此实例需要保留。
  // 直到销毁web_content_，以便此实例可以删除。
  // 临时文件。
  if (!temporary_files_.empty())
    return;

  if (listener_)
    listener_->FileSelectionCanceled();
  render_frame_host_ = nullptr;
  web_contents_ = nullptr;
  Release();
}

void FileSelectHelper::EnumerateDirectoryImpl(
    content::WebContents* tab,
    scoped_refptr<content::FileSelectListener> listener,
    const base::FilePath& path) {
  DCHECK(listener);
  DCHECK(!listener_);
  dialog_type_ = ui::SelectFileDialog::SELECT_NONE;
  web_contents_ = tab;
  listener_ = std::move(listener);
  // 因为该类将通知返回给RenderViewHost，所以它。
  // 呼叫者很难知道这个引用要保留多长时间。
  // 举个例子。我们在这里添加Ref()，以便在返回后保持实例处于活动状态。
  // 直到从枚举接收到最后一个回调为止。
  // 密码。此时，我们必须调用EnumerateDirectoryEnd()。
  AddRef();
  StartNewEnumeration(path);
}

// 当我们从枚举接收到最后一个回调时，将调用此方法。
// 密码。执行任何清理并释放我们在中添加的引用。
// EnumerateDirectoryImpl()。
void FileSelectHelper::EnumerateDirectoryEnd() {
  Release();
}

void FileSelectHelper::RenderWidgetHostDestroyed(
    content::RenderWidgetHost* widget_host) {
  render_frame_host_ = nullptr;
  DCHECK(observation_.IsObservingSource(widget_host));
  observation_.Reset();
}

void FileSelectHelper::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  if (!render_frame_host_)
    return;
  // 现在，|old_host|及其子项正在等待删除。不要给他们。
  // 文件访问超过此点。
  for (content::RenderFrameHost* host = render_frame_host_; host;
       host = host->GetParentOrOuterDocument()) {
    if (host == old_host) {
      render_frame_host_ = nullptr;
      return;
    }
  }
}

void FileSelectHelper::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == render_frame_host_)
    render_frame_host_ = nullptr;
}

void FileSelectHelper::WebContentsDestroyed() {
  render_frame_host_ = nullptr;
  web_contents_ = nullptr;
  CleanUp();
}

// 静电。
bool FileSelectHelper::IsAcceptTypeValid(const std::string& accept_type) {
  // TODO(Raymes)：这只做一些基本的检查，扩展到测试更多的案例。
  // 1个字符的接受类型将始终无效(可以是“.”在这种情况下。
  // 在MIME类型的情况下是扩展名或“/”)。
  std::string unused;
  if (accept_type.length() <= 1 ||
      base::ToLowerASCII(accept_type) != accept_type ||
      base::TrimWhitespaceASCII(accept_type, base::TRIM_ALL, &unused) !=
          base::TRIM_NONE) {
    return false;
  }
  return true;
}

// 静电
base::FilePath FileSelectHelper::GetSanitizedFileName(
    const base::FilePath& suggested_filename) {
  if (suggested_filename.empty())
    return base::FilePath();
  return net::GenerateFileName(
      GURL(), std::string(), std::string(), suggested_filename.AsUTF8Unsafe(),
      std::string(), l10n_util::GetStringUTF8(IDS_DEFAULT_DOWNLOAD_FILENAME));
}
