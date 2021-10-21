// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/session_preferences.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_context.h"

namespace electron {

// 静电。
int SessionPreferences::kLocatorKey = 0;

SessionPreferences::SessionPreferences(content::BrowserContext* context) {
  context->SetUserData(&kLocatorKey, base::WrapUnique(this));
}

SessionPreferences::~SessionPreferences() = default;

// 静电。
SessionPreferences* SessionPreferences::FromBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SessionPreferences*>(context->GetUserData(&kLocatorKey));
}

// 静电。
std::vector<base::FilePath> SessionPreferences::GetValidPreloads(
    content::BrowserContext* context) {
  std::vector<base::FilePath> result;

  if (auto* self = FromBrowserContext(context)) {
    for (const auto& preload : self->preloads()) {
      if (preload.IsAbsolute()) {
        result.emplace_back(preload);
      } else {
        LOG(ERROR) << "preload script must have absolute path: " << preload;
      }
    }
  }

  return result;
}

}  // 命名空间电子
