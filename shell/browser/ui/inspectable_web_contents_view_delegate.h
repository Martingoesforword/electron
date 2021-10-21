// 版权所有(C)2012 Chromium作者。版权所有。
// 版权所有(C)2013 Adam Roben&lt;adam@roben.org&gt;。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_
#define SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_

#include <string>

#include "ui/base/models/image_model.h"

namespace electron {

class InspectableWebContentsViewDelegate {
 public:
  virtual ~InspectableWebContentsViewDelegate() {}

  virtual void DevToolsFocused() {}
  virtual void DevToolsOpened() {}
  virtual void DevToolsClosed() {}
  virtual void DevToolsResized() {}

  // 返回DevTools窗口的图标。
  virtual ui::ImageModel GetDevToolsWindowIcon();

#if defined(OS_LINUX)
  // 在创建DevTools窗口时调用。
  virtual void GetDevToolsWindowWMClass(std::string* name,
                                        std::string* class_name) {}
#endif
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_DELEGATE_H_
