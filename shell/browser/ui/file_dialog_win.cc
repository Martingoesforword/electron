// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/file_dialog.h"

#include <windows.h>  // 必须先包含windows.h。

#include "base/win/shlwapi.h"  // NOLINT(BUILD/INCLUDE_ORDER)。

// CComPtr的atlbase.h。
#include <atlbase.h>  // NOLINT(BUILD/INCLUDE_ORDER)。

#include <shlobj.h>    // NOLINT(BUILD/INCLUDE_ORDER)。
#include <shobjidl.h>  // NOLINT(BUILD/INCLUDE_ORDER)。

#include "base/files/file_util.h"
#include "base/i18n/case_conversion.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/win/dialog_thread.h"
#include "shell/browser/unresponsive_suppressor.h"
#include "shell/common/gin_converters/file_path_converter.h"

namespace file_dialog {

DialogSettings::DialogSettings() = default;
DialogSettings::DialogSettings(const DialogSettings&) = default;
DialogSettings::~DialogSettings() = default;

namespace {

// 区分目录和常规文件。
bool IsDirectory(const base::FilePath& path) {
  base::File::Info file_info;
  return base::GetFileInfo(path, &file_info) ? file_info.is_directory
                                             : path.EndsWithSeparator();
}

void ConvertFilters(const Filters& filters,
                    std::vector<std::wstring>* buffer,
                    std::vector<COMDLG_FILTERSPEC>* filterspec) {
  if (filters.empty()) {
    COMDLG_FILTERSPEC spec = {L"All Files (*.*)", L"*.*"};
    filterspec->push_back(spec);
    return;
  }

  buffer->reserve(filters.size() * 2);
  for (const Filter& filter : filters) {
    COMDLG_FILTERSPEC spec;
    buffer->push_back(base::UTF8ToWide(filter.first));
    spec.pszName = buffer->back().c_str();

    std::vector<std::string> extensions(filter.second);
    for (std::string& extension : extensions)
      extension.insert(0, "*.");
    buffer->push_back(base::UTF8ToWide(base::JoinString(extensions, ";")));
    spec.pszSpec = buffer->back().c_str();

    filterspec->push_back(spec);
  }
}

static HRESULT GetFileNameFromShellItem(IShellItem* pShellItem,
                                        SIGDN type,
                                        LPWSTR lpstr,
                                        size_t cchLength) {
  assert(pShellItem != NULL);

  LPWSTR lpstrName = NULL;
  HRESULT hRet = pShellItem->GetDisplayName(type, &lpstrName);

  if (SUCCEEDED(hRet)) {
    if (wcslen(lpstrName) < cchLength) {
      wcscpy_s(lpstr, cchLength, lpstrName);
    } else {
      NOTREACHED();
      hRet = DISP_E_BUFFERTOOSMALL;
    }

    ::CoTaskMemFree(lpstrName);
  }

  return hRet;
}

static void SetDefaultFolder(IFileDialog* dialog,
                             const base::FilePath file_path) {
  std::wstring directory =
      IsDirectory(file_path) ? file_path.value() : file_path.DirName().value();

  ATL::CComPtr<IShellItem> folder_item;
  HRESULT hr = SHCreateItemFromParsingName(directory.c_str(), NULL,
                                           IID_PPV_ARGS(&folder_item));
  if (SUCCEEDED(hr))
    dialog->SetFolder(folder_item);
}

static HRESULT ShowFileDialog(IFileDialog* dialog,
                              const DialogSettings& settings) {
  electron::UnresponsiveSuppressor suppressor;
  HWND parent_window =
      settings.parent_window
          ? static_cast<electron::NativeWindowViews*>(settings.parent_window)
                ->GetAcceleratedWidget()
          : NULL;

  return dialog->Show(parent_window);
}

static void ApplySettings(IFileDialog* dialog, const DialogSettings& settings) {
  std::wstring file_part;

  if (!IsDirectory(settings.default_path))
    file_part = settings.default_path.BaseName().value();

  dialog->SetFileName(file_part.c_str());

  if (!settings.title.empty())
    dialog->SetTitle(base::UTF8ToWide(settings.title).c_str());

  if (!settings.button_label.empty())
    dialog->SetOkButtonLabel(base::UTF8ToWide(settings.button_label).c_str());

  std::vector<std::wstring> buffer;
  std::vector<COMDLG_FILTERSPEC> filterspec;
  ConvertFilters(settings.filters, &buffer, &filterspec);

  if (!filterspec.empty()) {
    dialog->SetFileTypes(filterspec.size(), filterspec.data());
  }

  // 默认情况下，如果文件类型为“*.*”，则文件名将添加*.*。在……里面。
  // 电子设备，我们将其禁用以获得更好的体验。
  // 
  // 来自微软新闻网：https://msdn.microsoft.com/en-us/library/windows/desktop/。
  // Bb775970(v=vs.85).aspx。
  // 
  // 如果未调用SetDefaultExtension，则对话框不会更新。
  // 当用户在文件对话框中选择新的文件类型时自动。
  // 
  // 我们将文件扩展名设置为要创建的第一个非通配符扩展名。
  // 确保对话框将自动更新文件扩展名。
  for (size_t i = 0; i < filterspec.size(); ++i) {
    if (std::wstring(filterspec[i].pszSpec) != L"*.*") {
      // SetFileTypeIndex被视为基于一的索引。
      dialog->SetFileTypeIndex(i + 1);
      dialog->SetDefaultExtension(filterspec[i].pszSpec);
      break;
    }
  }

  if (settings.default_path.IsAbsolute()) {
    SetDefaultFolder(dialog, settings.default_path);
  }
}

}  // 命名空间。

bool ShowOpenDialogSync(const DialogSettings& settings,
                        std::vector<base::FilePath>* paths) {
  ATL::CComPtr<IFileOpenDialog> file_open_dialog;
  HRESULT hr = file_open_dialog.CoCreateInstance(CLSID_FileOpenDialog);

  if (FAILED(hr))
    return false;

  DWORD options = FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST;
  if (settings.properties & OPEN_DIALOG_OPEN_DIRECTORY)
    options |= FOS_PICKFOLDERS;
  if (settings.properties & OPEN_DIALOG_MULTI_SELECTIONS)
    options |= FOS_ALLOWMULTISELECT;
  if (settings.properties & OPEN_DIALOG_SHOW_HIDDEN_FILES)
    options |= FOS_FORCESHOWHIDDEN;
  if (settings.properties & OPEN_DIALOG_PROMPT_TO_CREATE)
    options |= FOS_CREATEPROMPT;
  if (settings.properties & FILE_DIALOG_DONT_ADD_TO_RECENT)
    options |= FOS_DONTADDTORECENT;
  file_open_dialog->SetOptions(options);

  ApplySettings(file_open_dialog, settings);
  hr = ShowFileDialog(file_open_dialog, settings);
  if (FAILED(hr))
    return false;

  ATL::CComPtr<IShellItemArray> items;
  hr = file_open_dialog->GetResults(&items);
  if (FAILED(hr))
    return false;

  ATL::CComPtr<IShellItem> item;
  DWORD count = 0;
  hr = items->GetCount(&count);
  if (FAILED(hr))
    return false;

  paths->reserve(count);
  for (DWORD i = 0; i < count; ++i) {
    hr = items->GetItemAt(i, &item);
    if (FAILED(hr))
      return false;

    wchar_t file_name[MAX_PATH];
    hr = GetFileNameFromShellItem(item, SIGDN_FILESYSPATH, file_name,
                                  base::size(file_name));

    if (FAILED(hr))
      return false;

    paths->push_back(base::FilePath(file_name));
  }

  return true;
}

void ShowOpenDialog(const DialogSettings& settings,
                    gin_helper::Promise<gin_helper::Dictionary> promise) {
  auto done = [](gin_helper::Promise<gin_helper::Dictionary> promise,
                 bool success, std::vector<base::FilePath> result) {
    v8::Locker locker(promise.isolate());
    v8::HandleScope handle_scope(promise.isolate());
    gin::Dictionary dict = gin::Dictionary::CreateEmpty(promise.isolate());
    dict.Set("canceled", !success);
    dict.Set("filePaths", result);
    promise.Resolve(dict);
  };
  dialog_thread::Run(base::BindOnce(ShowOpenDialogSync, settings),
                     base::BindOnce(done, std::move(promise)));
}

bool ShowSaveDialogSync(const DialogSettings& settings, base::FilePath* path) {
  ATL::CComPtr<IFileSaveDialog> file_save_dialog;
  HRESULT hr = file_save_dialog.CoCreateInstance(CLSID_FileSaveDialog);
  if (FAILED(hr))
    return false;

  DWORD options = FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT;
  if (settings.properties & SAVE_DIALOG_SHOW_HIDDEN_FILES)
    options |= FOS_FORCESHOWHIDDEN;
  if (settings.properties & SAVE_DIALOG_DONT_ADD_TO_RECENT)
    options |= FOS_DONTADDTORECENT;

  file_save_dialog->SetOptions(options);
  ApplySettings(file_save_dialog, settings);
  hr = ShowFileDialog(file_save_dialog, settings);

  if (FAILED(hr))
    return false;

  CComPtr<IShellItem> pItem;
  hr = file_save_dialog->GetResult(&pItem);
  if (FAILED(hr))
    return false;

  PWSTR result_path = nullptr;
  hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &result_path);
  if (!SUCCEEDED(hr))
    return false;

  *path = base::FilePath(result_path);
  CoTaskMemFree(result_path);

  return true;
}

void ShowSaveDialog(const DialogSettings& settings,
                    gin_helper::Promise<gin_helper::Dictionary> promise) {
  auto done = [](gin_helper::Promise<gin_helper::Dictionary> promise,
                 bool success, base::FilePath result) {
    v8::Locker locker(promise.isolate());
    v8::HandleScope handle_scope(promise.isolate());
    gin::Dictionary dict = gin::Dictionary::CreateEmpty(promise.isolate());
    dict.Set("canceled", !success);
    dict.Set("filePath", result);
    promise.Resolve(dict);
  };
  dialog_thread::Run(base::BindOnce(ShowSaveDialogSync, settings),
                     base::BindOnce(done, std::move(promise)));
}

}  // 命名空间FILE_DIALOG
