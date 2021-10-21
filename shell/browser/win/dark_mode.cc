// 版权所有(C)2020 Microsoft Inc.保留所有权利。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#include "shell/browser/win/dark_mode.h"

#include <dwmapi.h>  // DwmSetWindowAttribute()。

#include "base/files/file_path.h"
#include "base/scoped_native_library.h"
#include "base/win/pe_image.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"

// 此命名空间包含最初来自。
// Https://github.com/ysc3839/win32-darkmode/。
// 由麻省理工学院执照和(C)Richard Yu管辖。
namespace {

// 1903年18362。
enum PreferredAppMode { Default, AllowDark, ForceDark, ForceLight, Max };

bool g_darkModeSupported = false;
bool g_darkModeEnabled = false;
DWORD g_buildNumber = 0;

enum WINDOWCOMPOSITIONATTRIB {
  WCA_USEDARKMODECOLORS = 26  // 内部版本超过18875。
};
struct WINDOWCOMPOSITIONATTRIBDATA {
  WINDOWCOMPOSITIONATTRIB Attrib;
  PVOID pvData;
  SIZE_T cbData;
};

using fnSetWindowCompositionAttribute =
    BOOL(WINAPI*)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;

bool IsHighContrast() {
  HIGHCONTRASTW highContrast = {sizeof(highContrast)};
  if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast),
                            &highContrast, FALSE))
    return highContrast.dwFlags & HCF_HIGHCONTRASTON;
  return false;
}

void RefreshTitleBarThemeColor(HWND hWnd, bool dark) {
  LONG ldark = dark;
  if (g_buildNumber >= 20161) {
    // DWMA_USE_IMERSIVE_DUSING_MODE=20。
    DwmSetWindowAttribute(hWnd, 20, &ldark, sizeof dark);
    return;
  }
  if (g_buildNumber >= 18363) {
    auto data = WINDOWCOMPOSITIONATTRIBDATA{WCA_USEDARKMODECOLORS, &ldark,
                                            sizeof ldark};
    _SetWindowCompositionAttribute(hWnd, &data);
    return;
  }
  DwmSetWindowAttribute(hWnd, 0x13, &ldark, sizeof ldark);
}

void InitDarkMode() {
  // 确认我们正在Windows的某个版本上运行。
  // 已知黑暗模式API的位置。
  auto* os_info = base::win::OSInfo::GetInstance();
  g_buildNumber = os_info->version_number().build;
  auto const version = os_info->version();
  if ((version < base::win::Version::WIN10_RS5) ||
      (version > base::win::Version::WIN10_20H1)) {
    return;
  }

  // 加载“SetWindowCompostionAttribute”，用于刷新TitleBarThemeColor()。
  _SetWindowCompositionAttribute =
      reinterpret_cast<decltype(_SetWindowCompositionAttribute)>(
          base::win::GetUser32FunctionPointer("SetWindowCompositionAttribute"));
  if (_SetWindowCompositionAttribute == nullptr) {
    return;
  }

  // 从uxheme.dll加载暗模式函数。
  // *刷新ImmersiveColorPolicyState()。
  // *ShouldAppsUseDarkMode()。
  // *AllowDarkModeForApp()。
  // *SetPferredAppMode()。
  // *AllowDarkModeForApp()(内部版本&lt;18362)。
  // *SetPferredAppMode()(Build&gt;=18362)。

  base::NativeLibrary uxtheme =
      base::PinSystemLibrary(FILE_PATH_LITERAL("uxtheme.dll"));
  if (!uxtheme) {
    return;
  }
  auto ux_pei = base::win::PEImage(uxtheme);
  auto get_ux_proc_from_ordinal = [&ux_pei](int ordinal, auto* setme) {
    FARPROC proc = ux_pei.GetProcAddress(reinterpret_cast<LPCSTR>(ordinal));
    *setme = reinterpret_cast<decltype(*setme)>(proc);
  };

  // 序数104。
  using fnRefreshImmersiveColorPolicyState = VOID(WINAPI*)();
  fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = {};
  get_ux_proc_from_ordinal(104, &_RefreshImmersiveColorPolicyState);

  // 序数132。
  using fnShouldAppsUseDarkMode = BOOL(WINAPI*)();
  fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = {};
  get_ux_proc_from_ordinal(132, &_ShouldAppsUseDarkMode);

  // 第135号，1809年。
  using fnAllowDarkModeForApp = BOOL(WINAPI*)(BOOL allow);
  fnAllowDarkModeForApp _AllowDarkModeForApp = {};

  // 第135号，1903年。
  typedef PreferredAppMode(WINAPI *
                           fnSetPreferredAppMode)(PreferredAppMode appMode);
  fnSetPreferredAppMode _SetPreferredAppMode = {};

  if (g_buildNumber < 18362) {
    get_ux_proc_from_ordinal(135, &_AllowDarkModeForApp);
  } else {
    get_ux_proc_from_ordinal(135, &_SetPreferredAppMode);
  }

  // 仅当我们找到函数时才支持暗模式。
  g_darkModeSupported = _RefreshImmersiveColorPolicyState &&
                        _ShouldAppsUseDarkMode &&
                        (_AllowDarkModeForApp || _SetPreferredAppMode);
  if (!g_darkModeSupported) {
    return;
  }

  // 初始设置：允许使用暗模式。
  if (_AllowDarkModeForApp) {
    _AllowDarkModeForApp(true);
  } else if (_SetPreferredAppMode) {
    _SetPreferredAppMode(AllowDark);
  }
  _RefreshImmersiveColorPolicyState();

  // 检查以查看当前是否启用了暗模式。
  g_darkModeEnabled = _ShouldAppsUseDarkMode() && !IsHighContrast();
}

}  // 命名空间。

namespace electron {

void EnsureInitialized() {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    ::InitDarkMode();
  }
}

bool IsDarkPreferred(ui::NativeTheme::ThemeSource theme_source) {
  switch (theme_source) {
    case ui::NativeTheme::ThemeSource::kForcedLight:
      return false;
    case ui::NativeTheme::ThemeSource::kForcedDark:
      return g_darkModeSupported;
    case ui::NativeTheme::ThemeSource::kSystem:
      return g_darkModeEnabled;
  }
}

namespace win {

void SetDarkModeForWindow(HWND hWnd,
                          ui::NativeTheme::ThemeSource theme_source) {
  EnsureInitialized();
  RefreshTitleBarThemeColor(hWnd, IsDarkPreferred(theme_source));
}

}  // 命名空间制胜。

}  // 命名空间电子
