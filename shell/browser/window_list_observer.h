// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_WINDOW_LIST_OBSERVER_H_
#define SHELL_BROWSER_WINDOW_LIST_OBSERVER_H_

#include "base/observer_list_types.h"

namespace electron {

class NativeWindow;

class WindowListObserver : public base::CheckedObserver {
 public:
  // 在将窗口添加到列表后立即调用。
  virtual void OnWindowAdded(NativeWindow* window) {}

  // 在窗口从列表中移除后立即调用。
  virtual void OnWindowRemoved(NativeWindow* window) {}

  // 当bepreunload处理程序取消窗口关闭时调用。
  virtual void OnWindowCloseCancelled(NativeWindow* window) {}

  // 在所有窗口关闭后立即调用。
  virtual void OnWindowAllClosed() {}

 protected:
  ~WindowListObserver() override {}
};

}  // 命名空间电子。

#endif  // Shell_Browser_Window_List_Observator_H_
