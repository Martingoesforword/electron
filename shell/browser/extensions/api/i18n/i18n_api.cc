// 版权所有(C)2020 Samuel Maddock&lt;Sam@samuelmaddock.com&gt;。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/api/i18n/i18n_api.h"

#include <string>
#include <vector>

#include "chrome/browser/browser_process.h"
#include "shell/common/extensions/api/i18n.h"

namespace GetAcceptLanguages = extensions::api::i18n::GetAcceptLanguages;

namespace extensions {

ExtensionFunction::ResponseAction I18nGetAcceptLanguagesFunction::Run() {
  auto locale = g_browser_process->GetApplicationLocale();
  std::vector<std::string> accept_languages = {locale};
  return RespondNow(
      ArgumentList(GetAcceptLanguages::Results::Create(accept_languages)));
}

}  // 命名空间扩展
