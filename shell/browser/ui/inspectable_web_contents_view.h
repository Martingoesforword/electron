// 版权所有(C)2012 Chromium作者。版权所有。
// 版权所有(C)2013 Adam Roben&lt;adam@roben.org&gt;。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_H_
#define SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_H_

#include <string>

#include "ui/gfx/native_widget_types.h"

class DevToolsContentsResizingStrategy;

#if defined(TOOLKIT_VIEWS)
namespace views {
class View;
}
#endif

namespace electron {

class InspectableWebContentsViewDelegate;

class InspectableWebContentsView {
 public:
  InspectableWebContentsView() {}
  virtual ~InspectableWebContentsView() {}

  // 代理管理自己的生活。
  void SetDelegate(InspectableWebContentsViewDelegate* delegate) {
    delegate_ = delegate;
  }
  InspectableWebContentsViewDelegate* GetDelegate() const { return delegate_; }

#if defined(TOOLKIT_VIEWS) && !defined(OS_MAC)
  // 返回附加了DevTools视图的容器控件。
  virtual views::View* GetView() = 0;

  // 返回web视图控件，该控件可由。
  // GetInitiallyFocusedView()将初始焦点设置为Web视图。
  virtual views::View* GetWebView() = 0;
#else
  virtual gfx::NativeView GetNativeView() const = 0;
#endif

  virtual void ShowDevTools(bool activate) = 0;
  // 隐藏DevTools视图。
  virtual void CloseDevTools() = 0;
  virtual bool IsDevToolsViewShowing() = 0;
  virtual bool IsDevToolsViewFocused() = 0;
  virtual void SetIsDocked(bool docked, bool activate) = 0;
  virtual void SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) = 0;
  virtual void SetTitle(const std::u16string& title) = 0;

 private:
  InspectableWebContentsViewDelegate* delegate_ = nullptr;  // 弱引用。
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_H_
