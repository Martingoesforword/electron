// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_WINDOW_LIST_H_
#define SHELL_BROWSER_WINDOW_LIST_H_

#include <vector>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/observer_list.h"

namespace electron {

class NativeWindow;
class WindowListObserver;

class WindowList {
 public:
  typedef std::vector<NativeWindow*> WindowVector;

  static WindowVector GetWindows();
  static bool IsEmpty();

  // 在与之关联的列表中添加或删除|Window|。
  static void AddWindow(NativeWindow* window);
  static void RemoveWindow(NativeWindow* window);

  // 当关闭被bepreunload处理程序取消时由窗口调用。
  static void WindowCloseCancelled(NativeWindow* window);

  // 在观察者列表中添加和删除|观察者|。
  static void AddObserver(WindowListObserver* observer);
  static void RemoveObserver(WindowListObserver* observer);

  // 关闭所有窗口。
  static void CloseAllWindows();

  // 摧毁所有窗户。
  static void DestroyAllWindows();

 private:
  static WindowList* GetInstance();

  WindowList();
  ~WindowList();

  // 此列表中窗口的矢量，按窗口的添加顺序排列。
  WindowVector windows_;

  // 每次添加窗口时都会收到通知的观察者列表，以及。
  // 删除所有窗口列表。
  static base::LazyInstance<base::ObserverList<WindowListObserver>>::Leaky
      observers_;

  static WindowList* instance_;

  DISALLOW_COPY_AND_ASSIGN(WindowList);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Window_List_H_
