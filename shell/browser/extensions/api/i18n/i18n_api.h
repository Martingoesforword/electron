// 版权所有(C)2020 Samuel Maddock&lt;Sam@samuelmaddock.com&gt;。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_API_I18N_I18N_API_H_
#define SHELL_BROWSER_EXTENSIONS_API_I18N_I18N_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {

class I18nGetAcceptLanguagesFunction : public ExtensionFunction {
  ~I18nGetAcceptLanguagesFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("i18n.getAcceptLanguages", I18N_GETACCEPTLANGUAGES)
};

}  // 命名空间扩展。

#endif  // Shell_Browser_Extensions_API_I18N_I18N_API_H_
