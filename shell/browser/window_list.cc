// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/window_list.h"

#include <algorithm>

#include "base/logging.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list_observer.h"

namespace {
template <typename T>
std::vector<base::WeakPtr<T>> ConvertToWeakPtrVector(std::vector<T*> raw_ptrs) {
  std::vector<base::WeakPtr<T>> converted_to_weak;
  converted_to_weak.reserve(raw_ptrs.size());
  for (auto* raw_ptr : raw_ptrs) {
    converted_to_weak.push_back(raw_ptr->GetWeakPtr());
  }
  return converted_to_weak;
}
}  // 命名空间。

namespace electron {

// 静电。
base::LazyInstance<base::ObserverList<WindowListObserver>>::Leaky
    WindowList::observers_ = LAZY_INSTANCE_INITIALIZER;

// 静电。
WindowList* WindowList::instance_ = nullptr;

// 静电。
WindowList* WindowList::GetInstance() {
  if (!instance_)
    instance_ = new WindowList;
  return instance_;
}

// 静电。
WindowList::WindowVector WindowList::GetWindows() {
  return GetInstance()->windows_;
}

// 静电。
bool WindowList::IsEmpty() {
  return GetInstance()->windows_.empty();
}

// 静电。
void WindowList::AddWindow(NativeWindow* window) {
  DCHECK(window);
  // 在相应的列表实例上推送|窗口|。
  WindowVector& windows = GetInstance()->windows_;
  windows.push_back(window);

  for (WindowListObserver& observer : observers_.Get())
    observer.OnWindowAdded(window);
}

// 静电。
void WindowList::RemoveWindow(NativeWindow* window) {
  WindowVector& windows = GetInstance()->windows_;
  windows.erase(std::remove(windows.begin(), windows.end(), window),
                windows.end());

  for (WindowListObserver& observer : observers_.Get())
    observer.OnWindowRemoved(window);

  if (windows.empty()) {
    for (WindowListObserver& observer : observers_.Get())
      observer.OnWindowAllClosed();
  }
}

// 静电。
void WindowList::WindowCloseCancelled(NativeWindow* window) {
  for (WindowListObserver& observer : observers_.Get())
    observer.OnWindowCloseCancelled(window);
}

// 静电。
void WindowList::AddObserver(WindowListObserver* observer) {
  observers_.Get().AddObserver(observer);
}

// 静电。
void WindowList::RemoveObserver(WindowListObserver* observer) {
  observers_.Get().RemoveObserver(observer);
}

// 静电。
void WindowList::CloseAllWindows() {
  std::vector<base::WeakPtr<NativeWindow>> weak_windows =
      ConvertToWeakPtrVector(GetInstance()->windows_);
#if defined(OS_MAC)
  std::reverse(weak_windows.begin(), weak_windows.end());
#endif
  for (const auto& window : weak_windows) {
    if (window && !window->IsClosed())
      window->Close();
  }
}

// 静电。
void WindowList::DestroyAllWindows() {
  std::vector<base::WeakPtr<NativeWindow>> weak_windows =
      ConvertToWeakPtrVector(GetInstance()->windows_);

  for (const auto& window : weak_windows) {
    if (window)
      window->CloseImmediately();
  }
}

WindowList::WindowList() = default;

WindowList::~WindowList() = default;

}  // 命名空间电子
