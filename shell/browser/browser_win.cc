// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/browser.h"

// 必须在其他包含之前。修复了&lt;shlwapi.h&gt;中错误的#定义。
#include "base/win/shlwapi.h"  // NOLINT(BUILD/INCLUDE_ORDER)。

#include <windows.h>  // NOLINT(BUILD/INCLUDE_ORDER)。

#include <atlbase.h>   // NOLINT(BUILD/INCLUDE_ORDER)。
#include <shlobj.h>    // NOLINT(BUILD/INCLUDE_ORDER)。
#include <shobjidl.h>  // NOLINT(BUILD/INCLUDE_ORDER)。

#include "base/base_paths.h"
#include "base/file_version_info.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/win/registry.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"
#include "chrome/browser/icon_manager.h"
#include "electron/electron_version.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/browser/badging/badge_manager.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/ui/message_box.h"
#include "shell/browser/ui/win/jump_list.h"
#include "shell/browser/window_list.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/arguments.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/skia_util.h"
#include "skia/ext/legacy_display_globals.h"
#include "third_party/skia/include/core/SkFont.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/keycodes/keyboard_code_conversion_win.h"
#include "ui/strings/grit/ui_strings.h"

namespace electron {

namespace {

bool GetProcessExecPath(std::wstring* exe) {
  base::FilePath path;
  if (!base::PathService::Get(base::FILE_EXE, &path)) {
    return false;
  }
  *exe = path.value();
  return true;
}

bool GetProtocolLaunchPath(gin::Arguments* args, std::wstring* exe) {
  if (!args->GetNext(exe) && !GetProcessExecPath(exe)) {
    return false;
  }

  // 读入可选参数Arg。
  std::vector<std::wstring> launch_args;
  if (args->GetNext(&launch_args) && !launch_args.empty())
    *exe = base::StringPrintf(L"\"%ls\" %ls \"%1\"", exe->c_str(),
                              base::JoinString(launch_args, L" ").c_str());
  else
    *exe = base::StringPrintf(L"\"%ls\" \"%1\"", exe->c_str());
  return true;
}

// Windows仅当给定方案的注册表为Internet方案时才将其视为Internet方案。
// 条目有一个“URL协议”键。选中此选项，否则我们允许ProgID。
// 用作自定义协议，这会导致安全错误。
bool IsValidCustomProtocol(const std::wstring& scheme) {
  if (scheme.empty())
    return false;
  base::win::RegKey cmd_key(HKEY_CLASSES_ROOT, scheme.c_str(), KEY_QUERY_VALUE);
  return cmd_key.Valid() && cmd_key.HasValue(L"URL Protocol");
}

// GetApplicationInfoForProtocol()的帮助器。
// 接受assoc_str。
// (https://docs.microsoft.com/en-us/windows/win32/api/shlwapi/ne-shlwapi-assocstr)。
// 并返回处理该协议的应用程序名称、图标和路径。
// 
// Windows8引入了一个新的协议-&gt;可执行绑定系统，它不能。
// 在下面实现的HKCR注册表子项方法中检索。我们打电话给。
// 而是使用新的仅限Win8的标志ASSOCF_IS_PROTOCOL代替AssocQueryString。
std::wstring GetAppInfoHelperForProtocol(ASSOCSTR assoc_str, const GURL& url) {
  const std::wstring url_scheme = base::ASCIIToWide(url.scheme());
  if (!IsValidCustomProtocol(url_scheme))
    return std::wstring();

  wchar_t out_buffer[1024];
  DWORD buffer_size = base::size(out_buffer);
  HRESULT hr =
      AssocQueryString(ASSOCF_IS_PROTOCOL, assoc_str, url_scheme.c_str(), NULL,
                       out_buffer, &buffer_size);
  if (FAILED(hr)) {
    DLOG(WARNING) << "AssocQueryString failed!";
    return std::wstring();
  }
  return std::wstring(out_buffer);
}

void OnIconDataAvailable(const base::FilePath& app_path,
                         const std::wstring& app_display_name,
                         gin_helper::Promise<gin_helper::Dictionary> promise,
                         gfx::Image icon) {
  if (!icon.IsEmpty()) {
    v8::HandleScope scope(promise.isolate());
    gin_helper::Dictionary dict =
        gin::Dictionary::CreateEmpty(promise.isolate());

    dict.Set("path", app_path);
    dict.Set("name", app_display_name);
    dict.Set("icon", icon);
    promise.Resolve(dict);
  } else {
    promise.RejectWithErrorMessage("Failed to get file icon.");
  }
}

std::wstring GetAppDisplayNameForProtocol(const GURL& url) {
  return GetAppInfoHelperForProtocol(ASSOCSTR_FRIENDLYAPPNAME, url);
}

std::wstring GetAppPathForProtocol(const GURL& url) {
  return GetAppInfoHelperForProtocol(ASSOCSTR_EXECUTABLE, url);
}

std::wstring GetAppForProtocolUsingRegistry(const GURL& url) {
  const std::wstring url_scheme = base::ASCIIToWide(url.scheme());
  if (!IsValidCustomProtocol(url_scheme))
    return std::wstring();

  // 首先，尝试提取应用程序的显示名称。
  std::wstring command_to_launch;
  base::win::RegKey cmd_key_name(HKEY_CLASSES_ROOT, url_scheme.c_str(),
                                 KEY_READ);
  if (cmd_key_name.ReadValue(NULL, &command_to_launch) == ERROR_SUCCESS &&
      !command_to_launch.empty()) {
    return command_to_launch;
  }

  // 否则，解析注册表中的命令行，并返回基本名称。
  // 程序路径(如果存在)的。
  const std::wstring cmd_key_path = url_scheme + L"\\shell\\open\\command";
  base::win::RegKey cmd_key_exe(HKEY_CLASSES_ROOT, cmd_key_path.c_str(),
                                KEY_READ);
  if (cmd_key_exe.ReadValue(NULL, &command_to_launch) == ERROR_SUCCESS) {
    base::CommandLine command_line(
        base::CommandLine::FromString(command_to_launch));
    return command_line.GetProgram().BaseName().value();
  }

  return std::wstring();
}

bool FormatCommandLineString(std::wstring* exe,
                             const std::vector<std::u16string>& launch_args) {
  if (exe->empty() && !GetProcessExecPath(exe)) {
    return false;
  }

  if (!launch_args.empty()) {
    std::u16string joined_launch_args = base::JoinString(launch_args, u" ");
    *exe = base::StringPrintf(L"%ls %ls", exe->c_str(),
                              base::as_wcstr(joined_launch_args));
  }

  return true;
}

// GetLoginItemSettings()的帮助器。
// 循环访问windows注册表路径中的所有条目并返回。
// 一个LaunchItem列表，其中包含指向我们的应用程序的匹配路径。
// 如果具有匹配路径的LaunchItem在。
// STARTUP_APPREED_KEY_PATH，将EXECUTABLE_Will_Launch_at_login设置为`true`。
std::vector<Browser::LaunchItem> GetLoginItemSettingsHelper(
    base::win::RegistryValueIterator* it,
    boolean* executable_will_launch_at_login,
    std::wstring scope,
    const Browser::LoginItemSettings& options) {
  std::vector<Browser::LaunchItem> launch_items;

  base::FilePath lookup_exe_path;
  if (options.path.empty()) {
    std::wstring process_exe_path;
    GetProcessExecPath(&process_exe_path);
    lookup_exe_path =
        base::CommandLine::FromString(process_exe_path).GetProgram();
  } else {
    lookup_exe_path =
        base::CommandLine::FromString(base::as_wcstr(options.path))
            .GetProgram();
  }

  if (!lookup_exe_path.empty()) {
    while (it->Valid()) {
      base::CommandLine registry_launch_cmd =
          base::CommandLine::FromString(it->Value());
      base::FilePath registry_launch_path = registry_launch_cmd.GetProgram();
      bool exe_match = base::FilePath::CompareEqualIgnoreCase(
          lookup_exe_path.value(), registry_launch_path.value());

      // 如果向量有匹配路径，则将启动项添加到向量(不区分大小写)。
      if (exe_match) {
        Browser::LaunchItem launch_item;
        launch_item.name = it->Name();
        launch_item.path = registry_launch_path.value();
        launch_item.args = registry_launch_cmd.GetArgs();
        launch_item.scope = scope;
        launch_item.enabled = true;

        // 如果存在匹配的密钥，请尝试更新Launch_item.Enabled。
        // StartupApproven注册表中的值条目。
        HKEY hkey;
        // StartupApproven注册表路径。
        LPCTSTR path = TEXT(
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApp"
            "roved\\Run");
        LONG res;
        if (scope == L"user") {
          res =
              RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &hkey);
        } else {
          res =
              RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_QUERY_VALUE, &hkey);
        }
        if (res == ERROR_SUCCESS) {
          DWORD type, size;
          wchar_t startup_binary[12];
          LONG result =
              RegQueryValueEx(hkey, it->Name(), nullptr, &type,
                              reinterpret_cast<BYTE*>(&startup_binary),
                              &(size = sizeof(startup_binary)));
          if (result == ERROR_SUCCESS) {
            if (type == REG_BINARY) {
              // 除此以外的任何其他二进制文件都表示该程序是。
              // 未设置为登录时启动。
              wchar_t binary_accepted[12] = {0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00};
              wchar_t binary_accepted_alt[12] = {0x02, 0x00, 0x00, 0x00,
                                                 0x00, 0x00, 0x00, 0x00,
                                                 0x00, 0x00, 0x00, 0x00};
              std::string reg_binary(reinterpret_cast<char*>(binary_accepted));
              std::string reg_binary_alt(
                  reinterpret_cast<char*>(binary_accepted_alt));
              std::string reg_startup_binary(
                  reinterpret_cast<char*>(startup_binary));
              launch_item.enabled = (reg_startup_binary == reg_binary) ||
                                    (reg_startup_binary == reg_binary_alt);
            }
          }
        }

        *executable_will_launch_at_login =
            *executable_will_launch_at_login || launch_item.enabled;
        launch_items.push_back(launch_item);
      }
      it->operator++();
    }
  }
  return launch_items;
}

std::unique_ptr<FileVersionInfo> FetchFileVersionInfo() {
  base::FilePath path;

  if (base::PathService::Get(base::FILE_EXE, &path)) {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    return FileVersionInfo::CreateFileVersionInfo(path);
  }
  return std::unique_ptr<FileVersionInfo>();
}

}  // 命名空间。

Browser::UserTask::UserTask() = default;
Browser::UserTask::UserTask(const UserTask&) = default;
Browser::UserTask::~UserTask() = default;

void GetFileIcon(const base::FilePath& path,
                 v8::Isolate* isolate,
                 base::CancelableTaskTracker* cancelable_task_tracker_,
                 const std::wstring app_display_name,
                 gin_helper::Promise<gin_helper::Dictionary> promise) {
  base::FilePath normalized_path = path.NormalizePathSeparators();
  IconLoader::IconSize icon_size = IconLoader::IconSize::LARGE;

  auto* icon_manager = ElectronBrowserMainParts::Get()->GetIconManager();
  gfx::Image* icon =
      icon_manager->LookupIconFromFilepath(normalized_path, icon_size, 1.0f);
  if (icon) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.Set("icon", *icon);
    dict.Set("name", app_display_name);
    dict.Set("path", normalized_path);
    promise.Resolve(dict);
  } else {
    icon_manager->LoadIcon(normalized_path, icon_size, 1.0f,
                           base::BindOnce(&OnIconDataAvailable, normalized_path,
                                          app_display_name, std::move(promise)),
                           cancelable_task_tracker_);
  }
}

void GetApplicationInfoForProtocolUsingRegistry(
    v8::Isolate* isolate,
    const GURL& url,
    gin_helper::Promise<gin_helper::Dictionary> promise,
    base::CancelableTaskTracker* cancelable_task_tracker_) {
  base::FilePath app_path;

  const std::wstring url_scheme = base::ASCIIToWide(url.scheme());
  if (!IsValidCustomProtocol(url_scheme)) {
    promise.RejectWithErrorMessage("invalid url_scheme");
    return;
  }
  std::wstring command_to_launch;
  const std::wstring cmd_key_path = url_scheme + L"\\shell\\open\\command";
  base::win::RegKey cmd_key_exe(HKEY_CLASSES_ROOT, cmd_key_path.c_str(),
                                KEY_READ);
  if (cmd_key_exe.ReadValue(NULL, &command_to_launch) == ERROR_SUCCESS) {
    base::CommandLine command_line(
        base::CommandLine::FromString(command_to_launch));
    app_path = command_line.GetProgram();
  } else {
    promise.RejectWithErrorMessage(
        "Unable to retrieve installation path to app");
    return;
  }
  const std::wstring app_display_name = GetAppForProtocolUsingRegistry(url);

  if (app_display_name.empty()) {
    promise.RejectWithErrorMessage(
        "Unable to retrieve application display name");
    return;
  }
  GetFileIcon(app_path, isolate, cancelable_task_tracker_, app_display_name,
              std::move(promise));
}

// 解析`Promise&lt;object&gt;`-解析包含以下内容的对象：
// *`icon`NativeImage-处理协议的应用的显示图标。
// *`path`String-处理该协议的APP的安装路径。
// *`name`String-处理协议的应用的显示名称。
void GetApplicationInfoForProtocolUsingAssocQuery(
    v8::Isolate* isolate,
    const GURL& url,
    gin_helper::Promise<gin_helper::Dictionary> promise,
    base::CancelableTaskTracker* cancelable_task_tracker_) {
  std::wstring app_path = GetAppPathForProtocol(url);

  if (app_path.empty()) {
    promise.RejectWithErrorMessage(
        "Unable to retrieve installation path to app");
    return;
  }

  std::wstring app_display_name = GetAppDisplayNameForProtocol(url);

  if (app_display_name.empty()) {
    promise.RejectWithErrorMessage("Unable to retrieve display name of app");
    return;
  }

  base::FilePath app_path_file_path = base::FilePath(app_path);
  GetFileIcon(app_path_file_path, isolate, cancelable_task_tracker_,
              app_display_name, std::move(promise));
}

void Browser::AddRecentDocument(const base::FilePath& path) {
  CComPtr<IShellItem> item;
  HRESULT hr = SHCreateItemFromParsingName(path.value().c_str(), NULL,
                                           IID_PPV_ARGS(&item));
  if (SUCCEEDED(hr)) {
    SHARDAPPIDINFO info;
    info.psi = item;
    info.pszAppID = GetAppUserModelID();
    SHAddToRecentDocs(SHARD_APPIDINFO, &info);
  }
}

void Browser::ClearRecentDocuments() {
  SHAddToRecentDocs(SHARD_APPIDINFO, nullptr);
}

void Browser::SetAppUserModelID(const std::wstring& name) {
  electron::SetAppUserModelID(name);
}

bool Browser::SetUserTasks(const std::vector<UserTask>& tasks) {
  JumpList jump_list(GetAppUserModelID());
  if (!jump_list.Begin())
    return false;

  JumpListCategory category;
  category.type = JumpListCategory::Type::kTasks;
  category.items.reserve(tasks.size());
  JumpListItem item;
  item.type = JumpListItem::Type::kTask;
  for (const auto& task : tasks) {
    item.title = task.title;
    item.path = task.program;
    item.arguments = task.arguments;
    item.icon_path = task.icon_path;
    item.icon_index = task.icon_index;
    item.description = task.description;
    item.working_dir = task.working_dir;
    category.items.push_back(item);
  }

  jump_list.AppendCategory(category);
  return jump_list.Commit();
}

bool Browser::RemoveAsDefaultProtocolClient(const std::string& protocol,
                                            gin::Arguments* args) {
  if (protocol.empty())
    return false;

  // 主注册表项。
  HKEY root = HKEY_CURRENT_USER;
  std::wstring keyPath = L"Software\\Classes\\";

  // 命令键。
  std::wstring wprotocol = base::UTF8ToWide(protocol);
  std::wstring shellPath = wprotocol + L"\\shell";
  std::wstring cmdPath = keyPath + shellPath + L"\\open\\command";

  base::win::RegKey classesKey;
  base::win::RegKey commandKey;

  if (FAILED(classesKey.Open(root, keyPath.c_str(), KEY_ALL_ACCESS)))
    // 类密钥不存在，这很令人担忧，但我想。
    // 我们不是默认处理程序。
    return true;

  if (FAILED(commandKey.Open(root, cmdPath.c_str(), KEY_ALL_ACCESS)))
    // 密钥甚至不存在，我们可以确认它没有设置。
    return true;

  std::wstring keyVal;
  if (FAILED(commandKey.ReadValue(L"", &keyVal)))
    // 未设置默认值，我们可以确认该值未设置。
    return true;

  std::wstring exe;
  if (!GetProtocolLaunchPath(args, &exe))
    return false;

  if (keyVal == exe) {
    // 让我们杀了这把钥匙
    if (FAILED(classesKey.DeleteKey(shellPath.c_str())))
      return false;

    // 让我们自己清理一下吧。
    base::win::RegKey protocolKey;
    std::wstring protocolPath = keyPath + wprotocol;

    if (SUCCEEDED(
            protocolKey.Open(root, protocolPath.c_str(), KEY_ALL_ACCESS))) {
      protocolKey.DeleteValue(L"URL Protocol");

      // 将默认值覆盖为空，我们不能立即将其删除。
      protocolKey.WriteValue(L"", L"");
      protocolKey.DeleteValue(L"");
    }

    // 如果现在为空，请删除整个密钥。
    classesKey.DeleteEmptyKey(wprotocol.c_str());

    return true;
  } else {
    return true;
  }
}

bool Browser::SetAsDefaultProtocolClient(const std::string& protocol,
                                         gin::Arguments* args) {
  // HKEY_CLASSES_ROOT。
  // $协议。
  // (默认)=“URL：$name”
  // URL协议=“”
  // 壳。
  // 打开。
  // 命令。
  // (默认)=“$COMMAND”“%1”
  // 
  // 但是，“HKEY_CLASSES_ROOT”键只能由。
  // 管理员用户。因此，我们改为写入“HKEY_CURRENT_USER\。
  // SOFTWARE\CLASSES“，由”HKEY_CLASSES_ROOT“继承。
  // 无论如何，都可以由非特权用户写入。

  if (protocol.empty())
    return false;

  std::wstring exe;
  if (!GetProtocolLaunchPath(args, &exe))
    return false;

  // 主注册表项。
  HKEY root = HKEY_CURRENT_USER;
  std::wstring keyPath = base::UTF8ToWide("Software\\Classes\\" + protocol);
  std::wstring urlDecl = base::UTF8ToWide("URL:" + protocol);

  // 命令键。
  std::wstring cmdPath = keyPath + L"\\shell\\open\\command";

  // 将信息写入注册表。
  base::win::RegKey key(root, keyPath.c_str(), KEY_ALL_ACCESS);
  if (FAILED(key.WriteValue(L"URL Protocol", L"")) ||
      FAILED(key.WriteValue(L"", urlDecl.c_str())))
    return false;

  base::win::RegKey commandKey(root, cmdPath.c_str(), KEY_ALL_ACCESS);
  if (FAILED(commandKey.WriteValue(L"", exe.c_str())))
    return false;

  return true;
}

bool Browser::IsDefaultProtocolClient(const std::string& protocol,
                                      gin::Arguments* args) {
  if (protocol.empty())
    return false;

  std::wstring exe;
  if (!GetProtocolLaunchPath(args, &exe))
    return false;

  // 主注册表项。
  HKEY root = HKEY_CURRENT_USER;
  std::wstring keyPath = base::UTF8ToWide("Software\\Classes\\" + protocol);

  // 命令键。
  std::wstring cmdPath = keyPath + L"\\shell\\open\\command";

  base::win::RegKey key;
  base::win::RegKey commandKey;
  if (FAILED(key.Open(root, keyPath.c_str(), KEY_ALL_ACCESS)))
    // 密钥不存在，我们可以确认没有设置。
    return false;

  if (FAILED(commandKey.Open(root, cmdPath.c_str(), KEY_ALL_ACCESS)))
    // 密钥不存在，我们可以确认没有设置。
    return false;

  std::wstring keyVal;
  if (FAILED(commandKey.ReadValue(L"", &keyVal)))
    // 未设置默认值，我们可以确认该值未设置。
    return false;

  // 默认值与当前文件路径相同。
  return keyVal == exe;
}

std::u16string Browser::GetApplicationNameForProtocol(const GURL& url) {
  // Windows 8或更高版本有一个新的协议关联查询。
  if (base::win::GetVersion() >= base::win::Version::WIN8) {
    std::wstring application_name = GetAppDisplayNameForProtocol(url);
    if (!application_name.empty())
      return base::WideToUTF16(application_name);
  }

  return base::WideToUTF16(GetAppForProtocolUsingRegistry(url));
}

v8::Local<v8::Promise> Browser::GetApplicationInfoForProtocol(
    v8::Isolate* isolate,
    const GURL& url) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // Windows 8或更高版本有一个新的协议关联查询。
  if (base::win::GetVersion() >= base::win::Version::WIN8) {
    GetApplicationInfoForProtocolUsingAssocQuery(
        isolate, url, std::move(promise), &cancelable_task_tracker_);
    return handle;
  }

  GetApplicationInfoForProtocolUsingRegistry(isolate, url, std::move(promise),
                                             &cancelable_task_tracker_);
  return handle;
}

bool Browser::SetBadgeCount(absl::optional<int> count) {
  absl::optional<std::string> badge_content;
  if (count.has_value() && count.value() == 0) {
    badge_content = absl::nullopt;
  } else {
    badge_content = badging::BadgeManager::GetBadgeString(count);
  }

  // 当徽章具有值时，有3种不同的情况：
  // 1.|内容|介于1和99之间=&gt;设置辅助功能文本。
  // 到多元通知计数(例如，4个未读通知)。
  // 2.|内容|大于99=&gt;将辅助功能文本设置为。
  // 多于|kMaxBadgeContent|未读通知，因此。
  // 辅助功能文本与徽章上显示的内容相匹配(例如更多。
  // 多于99个通知)。
  // 3.徽章设置为‘flag’=&gt;将辅助功能文本设置为。
  // 不太具体(例如，未读通知)。
  std::string badge_alt_string;
  if (count.has_value()) {
    badge_count_ = count.value();
    badge_alt_string = (uint64_t)badge_count_ <= badging::kMaxBadgeContent
                           // 案例1。
                           ? l10n_util::GetPluralStringFUTF8(
                                 IDS_BADGE_UNREAD_NOTIFICATIONS, badge_count_)
                           // 案例2。
                           : l10n_util::GetPluralStringFUTF8(
                                 IDS_BADGE_UNREAD_NOTIFICATIONS_SATURATED,
                                 badging::kMaxBadgeContent);
  } else {
    // 案例3。
    badge_alt_string =
        l10n_util::GetStringUTF8(IDS_BADGE_UNREAD_NOTIFICATIONS_UNSPECIFIED);
    badge_count_ = 0;
  }
  for (auto* window : WindowList::GetWindows()) {
    // 在Windows上，在找到的第一个窗口上设置徽章。
    UpdateBadgeContents(window->GetAcceleratedWidget(), badge_content,
                        badge_alt_string);
  }
  return true;
}

void Browser::UpdateBadgeContents(
    HWND hwnd,
    const absl::optional<std::string>& badge_content,
    const std::string& badge_alt_string) {
  SkBitmap badge;
  if (badge_content) {
    std::string content = badge_content.value();
    constexpr int kOverlayIconSize = 16;
    // 这是Windows 10平台工卡API使用的颜色。
    // 一致性。
    constexpr int kBackgroundColor = SkColorSetRGB(0x26, 0x25, 0x2D);
    constexpr int kForegroundColor = SK_ColorWHITE;
    constexpr int kRadius = kOverlayIconSize / 2;
    // 我们的内容和徽章边缘之间的最小差距。
    constexpr int kMinMargin = 3;
    // 渲染图标所需的空间量。
    constexpr int kMaxBounds = kOverlayIconSize - 2 * kMinMargin;
    constexpr int kMaxTextSize = 24;  // 文本的最大大小。
    constexpr int kMinTextSize = 7;   // 文本的最小尺寸。

    badge.allocN32Pixels(kOverlayIconSize, kOverlayIconSize);
    SkCanvas canvas(badge, skia::LegacyDisplayGlobals::GetSkSurfaceProps());

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(kBackgroundColor);

    canvas.clear(SK_ColorTRANSPARENT);
    canvas.drawCircle(kRadius, kRadius, kRadius, paint);

    paint.reset();
    paint.setColor(kForegroundColor);

    SkFont font;

    SkRect bounds;
    int text_size = kMaxTextSize;
    // 查找大于|kMinTextSize|的最大|TEXT_SIZE|，其中。
    // |内容|适合16x16px的图标，并留有页边距。
    do {
      font.setSize(text_size--);
      font.measureText(content.c_str(), content.size(), SkTextEncoding::kUTF8,
                       &bounds);
    } while (text_size >= kMinTextSize &&
             (bounds.width() > kMaxBounds || bounds.height() > kMaxBounds));

    canvas.drawSimpleText(
        content.c_str(), content.size(), SkTextEncoding::kUTF8,
        kRadius - bounds.width() / 2 - bounds.x(),
        kRadius - bounds.height() / 2 - bounds.y(), font, paint);
  }
  taskbar_host_.SetOverlayIcon(hwnd, badge, badge_alt_string);
}

void Browser::SetLoginItemSettings(LoginItemSettings settings) {
  std::wstring key_path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  base::win::RegKey key(HKEY_CURRENT_USER, key_path.c_str(), KEY_ALL_ACCESS);

  std::wstring startup_approved_key_path =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved"
      L"\\Run";
  base::win::RegKey startup_approved_key(
      HKEY_CURRENT_USER, startup_approved_key_path.c_str(), KEY_ALL_ACCESS);
  PCWSTR key_name =
      !settings.name.empty() ? settings.name.c_str() : GetAppUserModelID();

  if (settings.open_at_login) {
    std::wstring exe = base::UTF16ToWide(settings.path);
    if (FormatCommandLineString(&exe, settings.args)) {
      key.WriteValue(key_name, exe.c_str());

      if (settings.enabled) {
        startup_approved_key.DeleteValue(key_name);
      } else {
        HKEY hard_key;
        LPCTSTR path = TEXT(
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApp"
            "roved\\Run");
        LONG res =
            RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_ALL_ACCESS, &hard_key);

        if (res == ERROR_SUCCESS) {
          UCHAR disable_startup_binary[] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
          RegSetValueEx(hard_key, key_name, 0, REG_BINARY,
                        reinterpret_cast<const BYTE*>(disable_startup_binary),
                        sizeof(disable_startup_binary));
        }
      }
    }
  } else {
    // 如果登录时打开为假，请删除这两个值。
    startup_approved_key.DeleteValue(key_name);
    key.DeleteValue(key_name);
  }
}

Browser::LoginItemSettings Browser::GetLoginItemSettings(
    const LoginItemSettings& options) {
  LoginItemSettings settings;
  std::wstring keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  base::win::RegKey key(HKEY_CURRENT_USER, keyPath.c_str(), KEY_ALL_ACCESS);
  std::wstring keyVal;

  // 保留旧的openAtLogin行为。
  if (!FAILED(key.ReadValue(GetAppUserModelID(), &keyVal))) {
    std::wstring exe = base::UTF16ToWide(options.path);
    if (FormatCommandLineString(&exe, options.args)) {
      settings.open_at_login = keyVal == exe;
    }
  }

  // 迭代当前用户和计算机注册表并填充启动项。
  // 如果存在属性为Enabled==‘true’的启动条目，
  // 将EXECUTABLE_WILL_LAUNT_AT_LOGIN设置为‘true’。
  boolean executable_will_launch_at_login = false;
  std::vector<Browser::LaunchItem> launch_items;
  base::win::RegistryValueIterator hkcu_iterator(HKEY_CURRENT_USER,
                                                 keyPath.c_str());
  base::win::RegistryValueIterator hklm_iterator(HKEY_LOCAL_MACHINE,
                                                 keyPath.c_str());

  launch_items = GetLoginItemSettingsHelper(
      &hkcu_iterator, &executable_will_launch_at_login, L"user", options);
  std::vector<Browser::LaunchItem> launch_items_hklm =
      GetLoginItemSettingsHelper(&hklm_iterator,
                                 &executable_will_launch_at_login, L"machine",
                                 options);
  launch_items.insert(launch_items.end(), launch_items_hklm.begin(),
                      launch_items_hklm.end());

  settings.executable_will_launch_at_login = executable_will_launch_at_login;
  settings.launch_items = launch_items;
  return settings;
}

PCWSTR Browser::GetAppUserModelID() {
  return GetRawAppUserModelID();
}

std::string Browser::GetExecutableFileVersion() const {
  base::FilePath path;
  if (base::PathService::Get(base::FILE_EXE, &path)) {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    std::unique_ptr<FileVersionInfo> version_info = FetchFileVersionInfo();
    return base::UTF16ToUTF8(version_info->product_version());
  }

  return ELECTRON_VERSION_STRING;
}

std::string Browser::GetExecutableFileProductName() const {
  return GetApplicationName();
}

bool Browser::IsEmojiPanelSupported() {
  // Windows10的2018年春季更新及以上版本支持表情符号选择器。
  return base::win::GetVersion() >= base::win::Version::WIN10_RS4;
}

void Browser::ShowEmojiPanel() {
  // 这将发送Windows键+‘.’(Keydown和KeyUp事件)。
  // 使用“SendInput”是因为Windows需要接收这些事件，并且。
  // 打开表情符号选取器。
  INPUT input[4] = {};
  input[0].type = INPUT_KEYBOARD;
  input[0].ki.wVk = ui::WindowsKeyCodeForKeyboardCode(ui::VKEY_COMMAND);
  input[1].type = INPUT_KEYBOARD;
  input[1].ki.wVk = ui::WindowsKeyCodeForKeyboardCode(ui::VKEY_OEM_PERIOD);

  input[2].type = INPUT_KEYBOARD;
  input[2].ki.wVk = ui::WindowsKeyCodeForKeyboardCode(ui::VKEY_COMMAND);
  input[2].ki.dwFlags |= KEYEVENTF_KEYUP;
  input[3].type = INPUT_KEYBOARD;
  input[3].ki.wVk = ui::WindowsKeyCodeForKeyboardCode(ui::VKEY_OEM_PERIOD);
  input[3].ki.dwFlags |= KEYEVENTF_KEYUP;
  ::SendInput(4, input, sizeof(INPUT));
}

void Browser::ShowAboutPanel() {
  base::Value dict(base::Value::Type::DICTIONARY);
  std::string aboutMessage = "";
  gfx::ImageSkia image;

  // 从Windows.exe文件抓取默认值。
  std::unique_ptr<FileVersionInfo> exe_info = FetchFileVersionInfo();
  dict.SetStringKey("applicationName", exe_info->file_description());
  dict.SetStringKey("applicationVersion", exe_info->product_version());

  if (about_panel_options_.is_dict()) {
    dict.MergeDictionary(&about_panel_options_);
  }

  std::vector<std::string> stringOptions = {
      "applicationName", "applicationVersion", "copyright", "credits"};

  const std::string* str;
  for (std::string opt : stringOptions) {
    if ((str = dict.FindStringKey(opt))) {
      aboutMessage.append(*str).append("\r\n");
    }
  }

  if ((str = dict.FindStringKey("iconPath"))) {
    base::FilePath path = base::FilePath::FromUTF8Unsafe(*str);
    electron::util::PopulateImageSkiaRepsFromPath(&image, path);
  }

  electron::MessageBoxSettings settings = {};
  settings.message = aboutMessage;
  settings.icon = image;
  settings.type = electron::MessageBoxType::kInformation;
  electron::ShowMessageBoxSync(settings);
}

void Browser::SetAboutPanelOptions(base::DictionaryValue options) {
  about_panel_options_ = std::move(options);
}

}  // 命名空间电子
