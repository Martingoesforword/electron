// 版权所有(C)2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_UI_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_
#define SHELL_BROWSER_UI_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_

#include <memory>

#include "base/compiler_specific.h"
#include "chrome/browser/devtools/devtools_contents_resizing_strategy.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "ui/views/view.h"

namespace views {
class WebView;
class Widget;
class WidgetDelegate;
}  // 命名空间视图。

namespace electron {

class InspectableWebContents;

class InspectableWebContentsViewViews : public InspectableWebContentsView,
                                        public views::View {
 public:
  explicit InspectableWebContentsViewViews(
      InspectableWebContents* inspectable_web_contents);
  ~InspectableWebContentsViewViews() override;

  // InspectableWebContentsView：
  views::View* GetView() override;
  views::View* GetWebView() override;
  void ShowDevTools(bool activate) override;
  void CloseDevTools() override;
  bool IsDevToolsViewShowing() override;
  bool IsDevToolsViewFocused() override;
  void SetIsDocked(bool docked, bool activate) override;
  void SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) override;
  void SetTitle(const std::u16string& title) override;

  // 视图：：视图：
  void Layout() override;

  InspectableWebContents* inspectable_web_contents() {
    return inspectable_web_contents_;
  }

  const std::u16string& GetTitle() const { return title_; }

 private:
  // 拥有我们。
  InspectableWebContents* inspectable_web_contents_;

  std::unique_ptr<views::Widget> devtools_window_;
  views::WebView* devtools_window_web_view_ = nullptr;
  views::View* contents_web_view_ = nullptr;
  views::WebView* devtools_web_view_ = nullptr;

  DevToolsContentsResizingStrategy strategy_;
  bool devtools_visible_ = false;
  views::WidgetDelegate* devtools_window_delegate_ = nullptr;
  std::u16string title_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewViews);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_
