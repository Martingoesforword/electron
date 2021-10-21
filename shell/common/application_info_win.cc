// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/application_info.h"

#include <windows.h>  // 必须先包含windows.h。

#include <VersionHelpers.h>
#include <appmodel.h>
#include <shlobj.h>

#include <memory>

#include "base/file_version_info.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/string_util_win.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/win/scoped_hstring.h"

namespace electron {

namespace {

std::wstring g_app_user_model_id;
}

const wchar_t kAppUserModelIDFormat[] = L"electron.app.$1";

std::string GetApplicationName() {
  auto* module = GetModuleHandle(nullptr);
  std::unique_ptr<FileVersionInfo> info(
      FileVersionInfo::CreateFileVersionInfoForModule(module));
  return base::UTF16ToUTF8(info->product_name());
}

std::string GetApplicationVersion() {
  auto* module = GetModuleHandle(nullptr);
  std::unique_ptr<FileVersionInfo> info(
      FileVersionInfo::CreateFileVersionInfoForModule(module));
  return base::UTF16ToUTF8(info->product_version());
}

void SetAppUserModelID(const std::wstring& name) {
  g_app_user_model_id = name;
  SetCurrentProcessExplicitAppUserModelID(g_app_user_model_id.c_str());
}

PCWSTR GetRawAppUserModelID() {
  if (g_app_user_model_id.empty()) {
    PWSTR current_app_id;
    if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&current_app_id))) {
      g_app_user_model_id = current_app_id;
    } else {
      std::string name = GetApplicationName();
      std::wstring generated_app_id = base::ReplaceStringPlaceholders(
          kAppUserModelIDFormat, {base::UTF8ToWide(name)}, nullptr);
      SetAppUserModelID(generated_app_id);
    }
    CoTaskMemFree(current_app_id);
  }

  return g_app_user_model_id.c_str();
}

bool GetAppUserModelID(ScopedHString* app_id) {
  app_id->Reset(GetRawAppUserModelID());
  return app_id->success();
}

bool IsRunningInDesktopBridgeImpl() {
  if (IsWindows8OrGreater()) {
    // GetPackageFamilyName在Windows 7上不可用。
    using GetPackageFamilyNameFuncPtr = decltype(&GetPackageFamilyName);

    static bool initialize_get_package_family_name = true;
    static GetPackageFamilyNameFuncPtr get_package_family_namePtr = NULL;

    if (initialize_get_package_family_name) {
      initialize_get_package_family_name = false;
      HMODULE kernel32_base = GetModuleHandle(L"Kernel32.dll");
      if (!kernel32_base) {
        NOTREACHED() << std::string(" ") << std::string(__FUNCTION__)
                     << std::string("(): Can't open Kernel32.dll");
        return false;
      }

      get_package_family_namePtr =
          reinterpret_cast<GetPackageFamilyNameFuncPtr>(
              GetProcAddress(kernel32_base, "GetPackageFamilyName"));

      if (!get_package_family_namePtr) {
        return false;
      }
    }

    UINT32 length = PACKAGE_FAMILY_NAME_MAX_LENGTH;
    wchar_t packageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH];
    HANDLE proc = GetCurrentProcess();
    LONG result =
        (*get_package_family_namePtr)(proc, &length, packageFamilyName);

    return result == ERROR_SUCCESS;
  } else {
    return false;
  }
}

bool IsRunningInDesktopBridge() {
  static bool result = IsRunningInDesktopBridgeImpl();
  return result;
}

}  // 命名空间电子
