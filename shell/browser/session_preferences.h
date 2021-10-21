// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_SESSION_PREFERENCES_H_
#define SHELL_BROWSER_SESSION_PREFERENCES_H_

#include <vector>

#include "base/files/file_path.h"
#include "base/supports_user_data.h"

namespace content {
class BrowserContext;
}

namespace electron {

class SessionPreferences : public base::SupportsUserData::Data {
 public:
  static SessionPreferences* FromBrowserContext(
      content::BrowserContext* context);
  static std::vector<base::FilePath> GetValidPreloads(
      content::BrowserContext* context);

  explicit SessionPreferences(content::BrowserContext* context);
  ~SessionPreferences() override;

  void set_preloads(const std::vector<base::FilePath>& preloads) {
    preloads_ = preloads;
  }
  const std::vector<base::FilePath>& preloads() const { return preloads_; }

 private:
  // 用户数据密钥。
  static int kLocatorKey;

  std::vector<base::FilePath> preloads_;
};

}  // 命名空间电子。

#endif  // Shell_Browser_Session_Preferences_H_
