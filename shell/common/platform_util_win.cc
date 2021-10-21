// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/platform_util.h"

#include <windows.h>  // 必须先包含windows.h。

#include <atlbase.h>
#include <comdef.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wrl/client.h>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/win/registry.h"
#include "base/win/scoped_co_mem.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/windows_version.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"
#include "shell/common/electron_paths.h"
#include "ui/base/win/shell.h"
#include "url/gurl.h"

namespace {

// 需要IFileOperationProgressSink的COM实现，以便我们可以。
// 在删除之前预先检查文件，以确保它们可以移动到。
// 回收站。
class DeleteFileProgressSink : public IFileOperationProgressSink {
 public:
  DeleteFileProgressSink();
  virtual ~DeleteFileProgressSink() = default;

 private:
  ULONG STDMETHODCALLTYPE AddRef(void) override;
  ULONG STDMETHODCALLTYPE Release(void) override;
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                           LPVOID* ppvObj) override;
  HRESULT STDMETHODCALLTYPE StartOperations(void) override;
  HRESULT STDMETHODCALLTYPE FinishOperations(HRESULT) override;
  HRESULT STDMETHODCALLTYPE PreRenameItem(DWORD, IShellItem*, LPCWSTR) override;
  HRESULT STDMETHODCALLTYPE
  PostRenameItem(DWORD, IShellItem*, LPCWSTR, HRESULT, IShellItem*) override;
  HRESULT STDMETHODCALLTYPE PreMoveItem(DWORD,
                                        IShellItem*,
                                        IShellItem*,
                                        LPCWSTR) override;
  HRESULT STDMETHODCALLTYPE PostMoveItem(DWORD,
                                         IShellItem*,
                                         IShellItem*,
                                         LPCWSTR,
                                         HRESULT,
                                         IShellItem*) override;
  HRESULT STDMETHODCALLTYPE PreCopyItem(DWORD,
                                        IShellItem*,
                                        IShellItem*,
                                        LPCWSTR) override;
  HRESULT STDMETHODCALLTYPE PostCopyItem(DWORD,
                                         IShellItem*,
                                         IShellItem*,
                                         LPCWSTR,
                                         HRESULT,
                                         IShellItem*) override;
  HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD, IShellItem*) override;
  HRESULT STDMETHODCALLTYPE PostDeleteItem(DWORD,
                                           IShellItem*,
                                           HRESULT,
                                           IShellItem*) override;
  HRESULT STDMETHODCALLTYPE PreNewItem(DWORD, IShellItem*, LPCWSTR) override;
  HRESULT STDMETHODCALLTYPE PostNewItem(DWORD,
                                        IShellItem*,
                                        LPCWSTR,
                                        LPCWSTR,
                                        DWORD,
                                        HRESULT,
                                        IShellItem*) override;
  HRESULT STDMETHODCALLTYPE UpdateProgress(UINT, UINT) override;
  HRESULT STDMETHODCALLTYPE ResetTimer(void) override;
  HRESULT STDMETHODCALLTYPE PauseTimer(void) override;
  HRESULT STDMETHODCALLTYPE ResumeTimer(void) override;

  ULONG m_cRef;
};

DeleteFileProgressSink::DeleteFileProgressSink() {
  m_cRef = 0;
}

HRESULT DeleteFileProgressSink::PreDeleteItem(DWORD dwFlags, IShellItem*) {
  if (!(dwFlags & TSF_DELETE_RECYCLE_IF_POSSIBLE)) {
    // 不会为无法设置的项目设置TSF_DELETE_RECYCLE_IF_PUBLICE。
    // 回收利用。在本例中，我们中止删除操作。这个泡泡。
    // 向上并停止IFileOperation中的删除。
    return E_ABORT;
  }
  // 如果成功，则返回S_OK，否则返回错误值。在此情况下，
  // 错误值、删除操作和所有后续操作挂起。
  // 对IFileOperation的调用被取消。
  return S_OK;
}

HRESULT DeleteFileProgressSink::QueryInterface(REFIID riid, LPVOID* ppvObj) {
  // 始终将参数设置为NULL，首先进行验证。
  if (!ppvObj)
    return E_INVALIDARG;
  *ppvObj = nullptr;
  if (riid == IID_IUnknown || riid == IID_IFileOperationProgressSink) {
    // 递增引用计数并返回指针。
    *ppvObj = reinterpret_cast<IUnknown*>(this);
    AddRef();
    return NOERROR;
  }
  return E_NOINTERFACE;
}

ULONG DeleteFileProgressSink::AddRef() {
  InterlockedIncrement(&m_cRef);
  return m_cRef;
}

ULONG DeleteFileProgressSink::Release() {
  // 递减对象的内部计数器。
  ULONG ulRefCount = InterlockedDecrement(&m_cRef);
  if (0 == m_cRef) {
    delete this;
  }
  return ulRefCount;
}

HRESULT DeleteFileProgressSink::StartOperations() {
  return S_OK;
}

HRESULT DeleteFileProgressSink::FinishOperations(HRESULT) {
  return S_OK;
}

HRESULT DeleteFileProgressSink::PreRenameItem(DWORD, IShellItem*, LPCWSTR) {
  return S_OK;
}

HRESULT DeleteFileProgressSink::PostRenameItem(DWORD,
                                               IShellItem*,
                                               __RPC__in_string LPCWSTR,
                                               HRESULT,
                                               IShellItem*) {
  return E_NOTIMPL;
}

HRESULT DeleteFileProgressSink::PreMoveItem(DWORD,
                                            IShellItem*,
                                            IShellItem*,
                                            LPCWSTR) {
  return E_NOTIMPL;
}

HRESULT DeleteFileProgressSink::PostMoveItem(DWORD,
                                             IShellItem*,
                                             IShellItem*,
                                             LPCWSTR,
                                             HRESULT,
                                             IShellItem*) {
  return E_NOTIMPL;
}

HRESULT DeleteFileProgressSink::PreCopyItem(DWORD,
                                            IShellItem*,
                                            IShellItem*,
                                            LPCWSTR) {
  return E_NOTIMPL;
}

HRESULT DeleteFileProgressSink::PostCopyItem(DWORD,
                                             IShellItem*,
                                             IShellItem*,
                                             LPCWSTR,
                                             HRESULT,
                                             IShellItem*) {
  return E_NOTIMPL;
}

HRESULT DeleteFileProgressSink::PostDeleteItem(DWORD,
                                               IShellItem*,
                                               HRESULT,
                                               IShellItem*) {
  return S_OK;
}

HRESULT DeleteFileProgressSink::PreNewItem(DWORD dwFlags,
                                           IShellItem*,
                                           LPCWSTR) {
  return E_NOTIMPL;
}

HRESULT DeleteFileProgressSink::PostNewItem(DWORD,
                                            IShellItem*,
                                            LPCWSTR,
                                            LPCWSTR,
                                            DWORD,
                                            HRESULT,
                                            IShellItem*) {
  return E_NOTIMPL;
}

HRESULT DeleteFileProgressSink::UpdateProgress(UINT, UINT) {
  return S_OK;
}

HRESULT DeleteFileProgressSink::ResetTimer() {
  return S_OK;
}

HRESULT DeleteFileProgressSink::PauseTimer() {
  return S_OK;
}

HRESULT DeleteFileProgressSink::ResumeTimer() {
  return S_OK;
}

std::string OpenExternalOnWorkerThread(
    const GURL& url,
    const platform_util::OpenExternalOptions& options) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);
  // 引用输入方案以确保命令没有。
  // 外部程序意外的参数。此URL应该已经。
  // 已经逃脱了。
  std::wstring escaped_url =
      L"\"" + base::UTF8ToWide(net::EscapeExternalHandlerValue(url.spec())) +
      L"\"";
  std::wstring working_dir = options.working_dir.value();

  if (reinterpret_cast<ULONG_PTR>(
          ShellExecuteW(nullptr, L"open", escaped_url.c_str(), nullptr,
                        working_dir.empty() ? nullptr : working_dir.c_str(),
                        SW_SHOWNORMAL)) <= 32) {
    return "Failed to open";
  }
  return "";
}

void ShowItemInFolderOnWorkerThread(const base::FilePath& full_path) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);
  base::win::ScopedCOMInitializer com_initializer;
  if (!com_initializer.Succeeded())
    return;

  base::FilePath dir = full_path.DirName().AsEndingWithSeparator();
  // 如果目录是“C：”，则ParseDisplayName将失败，它必须是“C：\\”。
  if (dir.empty())
    return;

  Microsoft::WRL::ComPtr<IShellFolder> desktop;
  HRESULT hr = SHGetDesktopFolder(desktop.GetAddressOf());
  if (FAILED(hr))
    return;

  base::win::ScopedCoMem<ITEMIDLIST> dir_item;
  hr = desktop->ParseDisplayName(NULL, NULL,
                                 const_cast<wchar_t*>(dir.value().c_str()),
                                 NULL, &dir_item, NULL);
  if (FAILED(hr))
    return;

  base::win::ScopedCoMem<ITEMIDLIST> file_item;
  hr = desktop->ParseDisplayName(
      NULL, NULL, const_cast<wchar_t*>(full_path.value().c_str()), NULL,
      &file_item, NULL);
  if (FAILED(hr))
    return;

  const ITEMIDLIST* highlight[] = {file_item};
  hr = SHOpenFolderAndSelectItems(dir_item, base::size(highlight), highlight,
                                  NULL);
  if (FAILED(hr)) {
    // 在某些系统上，上述调用神秘地失败，并显示“file not。
    // Found“，即使文件在那里。在这些情况下，ShellExecute()。
    // 似乎起到了后备作用(尽管它不会选择文件)。
    if (hr == ERROR_FILE_NOT_FOUND) {
      ShellExecute(NULL, L"open", dir.value().c_str(), NULL, NULL, SW_SHOW);
    } else {
      LOG(WARNING) << " " << __func__ << "(): Can't open full_path = \""
                   << full_path.value() << "\""
                   << " hr = " << logging::SystemErrorCodeToString(hr);
    }
  }
}

std::string OpenPathOnThread(const base::FilePath& full_path) {
  // 可以导致交互式对话。
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);
  bool success;
  if (base::DirectoryExists(full_path))
    success = ui::win::OpenFolderViaShell(full_path);
  else
    success = ui::win::OpenFileViaShell(full_path);

  return success ? "" : "Failed to open path";
}

}  // 命名空间。

namespace platform_util {

void ShowItemInFolder(const base::FilePath& full_path) {
  base::ThreadPool::CreateCOMSTATaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING})
      ->PostTask(FROM_HERE,
                 base::BindOnce(&ShowItemInFolderOnWorkerThread, full_path));
}

void OpenPath(const base::FilePath& full_path, OpenCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::PostTaskAndReplyWithResult(
      base::ThreadPool::CreateCOMSTATaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_BLOCKING})
          .get(),
      FROM_HERE, base::BindOnce(&OpenPathOnThread, full_path),
      std::move(callback));
}

void OpenExternal(const GURL& url,
                  const OpenExternalOptions& options,
                  OpenCallback callback) {
  base::PostTaskAndReplyWithResult(
      base::ThreadPool::CreateCOMSTATaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_BLOCKING})
          .get(),
      FROM_HERE, base::BindOnce(&OpenExternalOnWorkerThread, url, options),
      std::move(callback));
}

bool MoveItemToTrashWithError(const base::FilePath& path,
                              bool delete_on_fail,
                              std::string* error) {
  Microsoft::WRL::ComPtr<IFileOperation> pfo;
  if (FAILED(::CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL,
                                IID_PPV_ARGS(&pfo)))) {
    *error = "Failed to create FileOperation instance";
    return false;
  }

  // 已为受UAC保护的文件启用提升提示。这会重写。
  // 静默、NO_UI和NOERRORUI标志。

  if (base::win::GetVersion() >= base::win::Version::WIN8) {
    // Windows 8引入了标志RECYCLEONDELETE，不建议使用。
    // 你好，支持ADDUNDORECORD。
    if (FAILED(pfo->SetOperationFlags(
            FOF_NO_UI | FOFX_ADDUNDORECORD | FOF_NOERRORUI | FOF_SILENT |
            FOFX_SHOWELEVATIONPROMPT | FOFX_RECYCLEONDELETE))) {
      *error = "Failed to set operation flags";
      return false;
    }
  } else {
    // 对于Windows 7和Vista，RecycleOnDelete是默认行为。
    if (FAILED(pfo->SetOperationFlags(FOF_NO_UI | FOF_ALLOWUNDO |
                                      FOF_NOERRORUI | FOF_SILENT |
                                      FOFX_SHOWELEVATIONPROMPT))) {
      *error = "Failed to set operation flags";
      return false;
    }
  }

  // 从提供的源路径创建IShellItem。
  Microsoft::WRL::ComPtr<IShellItem> delete_item;
  if (FAILED(SHCreateItemFromParsingName(
          path.value().c_str(), NULL,
          IID_PPV_ARGS(delete_item.GetAddressOf())))) {
    *error = "Failed to parse path";
    return false;
  }

  Microsoft::WRL::ComPtr<IFileOperationProgressSink> delete_sink(
      new DeleteFileProgressSink);
  if (!delete_sink) {
    *error = "Failed to create delete sink";
    return false;
  }

  BOOL pfAnyOperationsAborted;

  // 处理排队的命令DeleteItem。这将触发。
  // 要检查回收站的DeleteFileProgressSink。
  if (!SUCCEEDED(pfo->DeleteItem(delete_item.Get(), delete_sink.Get()))) {
    *error = "Failed to enqueue DeleteItem command";
    return false;
  }

  if (!SUCCEEDED(pfo->PerformOperations())) {
    *error = "Failed to perform delete operation";
    return false;
  }

  if (!SUCCEEDED(pfo->GetAnyOperationsAborted(&pfAnyOperationsAborted))) {
    *error = "Failed to check operation status";
    return false;
  }

  if (pfAnyOperationsAborted) {
    *error = "Operation was aborted";
    return false;
  }

  return true;
}

namespace internal {

bool PlatformTrashItem(const base::FilePath& full_path, std::string* error) {
  return MoveItemToTrashWithError(full_path, false, error);
}

}  // 命名空间内部。

bool GetFolderPath(int key, base::FilePath* result) {
  wchar_t system_buffer[MAX_PATH];

  switch (key) {
    case electron::DIR_RECENT:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_RECENT, NULL, SHGFP_TYPE_CURRENT,
                                 system_buffer))) {
        return false;
      }
      *result = base::FilePath(system_buffer);
      break;
  }

  return true;
}

void Beep() {
  MessageBeep(MB_OK);
}

}  // 命名空间Platform_util
