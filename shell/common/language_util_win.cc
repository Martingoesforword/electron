// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/language_util.h"

#include <roapi.h>
#include <windows.system.userprofile.h>
#include <wrl.h>

#include "base/strings/sys_string_conversions.h"
#include "base/win/core_winrt_util.h"
#include "base/win/i18n.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"

namespace electron {

bool GetPreferredLanguagesUsingGlobalization(
    std::vector<std::wstring>* languages) {
  if (base::win::GetVersion() < base::win::Version::WIN10)
    return false;
  if (!base::win::ResolveCoreWinRTDelayload() ||
      !base::win::ScopedHString::ResolveCoreWinRTStringDelayload())
    return false;

  base::win::ScopedHString guid = base::win::ScopedHString::Create(
      RuntimeClass_Windows_System_UserProfile_GlobalizationPreferences);
  Microsoft::WRL::ComPtr<
      ABI::Windows::System::UserProfile::IGlobalizationPreferencesStatics>
      prefs;

  HRESULT hr =
      base::win::RoGetActivationFactory(guid.get(), IID_PPV_ARGS(&prefs));
  if (FAILED(hr))
    return false;

  ABI::Windows::Foundation::Collections::IVectorView<HSTRING>* langs;
  hr = prefs->get_Languages(&langs);
  if (FAILED(hr))
    return false;

  unsigned size;
  hr = langs->get_Size(&size);
  if (FAILED(hr))
    return false;

  for (unsigned i = 0; i < size; ++i) {
    HSTRING hstr;
    hr = langs->GetAt(i, &hstr);
    if (SUCCEEDED(hr)) {
      base::WStringPiece str = base::win::ScopedHString(hstr).Get();
      languages->emplace_back(str.data(), str.size());
    }
  }

  return true;
}

std::vector<std::string> GetPreferredLanguages() {
  std::vector<std::wstring> languages16;

  // 尝试使用Windows 10或更高版本上可用的API，该API。
  // 返回语言首选项的完整列表。
  if (!GetPreferredLanguagesUsingGlobalization(&languages16)) {
    base::win::i18n::GetThreadPreferredUILanguageList(&languages16);
  }

  std::vector<std::string> languages;
  for (const auto& language : languages16) {
    languages.push_back(base::SysWideToUTF8(language));
  }
  return languages;
}

}  // 命名空间电子
