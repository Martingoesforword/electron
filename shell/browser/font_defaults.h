// 版权所有(C)2018 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_FONT_DEFAULTS_H_
#define SHELL_BROWSER_FONT_DEFAULTS_H_

namespace blink {
namespace web_pref {
struct WebPreferences;
}  // 命名空间web_pref。
}  // 命名空间闪烁。

namespace electron {

void SetFontDefaults(blink::web_pref::WebPreferences* prefs);

}  // 命名空间电子。

#endif  // Shell_Browser_FONT_DEFAULTS_H_
