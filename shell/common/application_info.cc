// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/application_info.h"

#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "chrome/common/chrome_version.h"
#include "content/public/common/user_agent.h"
#include "electron/electron_version.h"
#include "shell/browser/browser.h"

namespace electron {

std::string& OverriddenApplicationName() {
  static base::NoDestructor<std::string> overridden_application_name;
  return *overridden_application_name;
}

std::string& OverriddenApplicationVersion() {
  static base::NoDestructor<std::string> overridden_application_version;
  return *overridden_application_version;
}

std::string GetPossiblyOverriddenApplicationName() {
  std::string ret = OverriddenApplicationName();
  if (!ret.empty())
    return ret;
  return GetApplicationName();
}

std::string GetApplicationUserAgent() {
  // 构造用户代理字符串。
  Browser* browser = Browser::Get();
  std::string name, user_agent;
  if (!base::RemoveChars(browser->GetName(), " ", &name))
    name = browser->GetName();
  if (name == ELECTRON_PRODUCT_NAME) {
    user_agent = "Chrome/" CHROME_VERSION_STRING " " ELECTRON_PRODUCT_NAME
                 "/" ELECTRON_VERSION_STRING;
  } else {
    user_agent = base::StringPrintf(
        "%s/%s Chrome/%s " ELECTRON_PRODUCT_NAME "/" ELECTRON_VERSION_STRING,
        name.c_str(), browser->GetVersion().c_str(), CHROME_VERSION_STRING);
  }
  return content::BuildUserAgentFromProduct(user_agent);
}

}  // 命名空间电子
