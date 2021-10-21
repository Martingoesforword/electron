// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_AUTO_UPDATER_H_
#define SHELL_BROWSER_AUTO_UPDATER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "build/build_config.h"

namespace base {
class Time;
}

namespace gin {
class Arguments;
}

namespace auto_updater {

class Delegate {
 public:
  // 发生错误。
  virtual void OnError(const std::string& message) {}

  virtual void OnError(const std::string& message,
                       const int code,
                       const std::string& domain) {}

  // 正在检查是否有更新。
  virtual void OnCheckingForUpdate() {}

  // 有可用的更新，正在下载。
  virtual void OnUpdateAvailable() {}

  // 没有可用的更新。
  virtual void OnUpdateNotAvailable() {}

  // 已经下载了一个新的更新。
  virtual void OnUpdateDownloaded(const std::string& release_notes,
                                  const std::string& release_name,
                                  const base::Time& release_date,
                                  const std::string& update_url) {}

 protected:
  virtual ~Delegate() {}
};

class AutoUpdater {
 public:
  typedef std::map<std::string, std::string> HeaderMap;

  // 获取/设置委托。
  static Delegate* GetDelegate();
  static void SetDelegate(Delegate* delegate);

  static std::string GetFeedURL();
  // FIXME(Zcbenz)：我们不应该在这个文件中使用V8，这个方法应该只。
  // 接受C++struct作为参数，ATOM_API_AUTO_updater.cc负责。
  // 用于从JavaScript解析参数。
  static void SetFeedURL(gin::Arguments* args);
  static void CheckForUpdates();
  static void QuitAndInstall();

 private:
  static Delegate* delegate_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(AutoUpdater);
};

}  // 命名空间自动更新程序(_U)。

#endif  // Shell_Browser_AUTO_UPDATER_H_
