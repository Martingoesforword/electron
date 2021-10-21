// 版权所有(C)2012 Chromium作者。版权所有。
// 版权所有(C)2013 Adam Roben&lt;adam@roben.org&gt;。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_
#define SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_

#include <string>

#include "base/files/file_path.h"

namespace electron {

class InspectableWebContentsDelegate {
 public:
  virtual ~InspectableWebContentsDelegate() {}

  // 由DevTools的WebContents请求。
  virtual void DevToolsReloadPage() {}
  virtual void DevToolsSaveToFile(const std::string& url,
                                  const std::string& content,
                                  bool save_as) {}
  virtual void DevToolsAppendToFile(const std::string& url,
                                    const std::string& content) {}
  virtual void DevToolsRequestFileSystems() {}
  virtual void DevToolsAddFileSystem(const std::string& type,
                                     const base::FilePath& file_system_path) {}
  virtual void DevToolsRemoveFileSystem(
      const base::FilePath& file_system_path) {}
  virtual void DevToolsIndexPath(int request_id,
                                 const std::string& file_system_path,
                                 const std::string& excluded_folders) {}
  virtual void DevToolsStopIndexing(int request_id) {}
  virtual void DevToolsSearchInPath(int request_id,
                                    const std::string& file_system_path,
                                    const std::string& query) {}
  virtual void DevToolsSetEyeDropperActive(bool active) {}
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_DELEGATE_H_
