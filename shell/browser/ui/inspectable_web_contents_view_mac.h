// 版权所有(C)2012 Chromium作者。版权所有。
// 版权所有(C)2013 Adam Roben&lt;adam@roben.org&gt;。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_
#define SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_

#include "shell/browser/ui/inspectable_web_contents_view.h"

#include "base/mac/scoped_nsobject.h"

@class ElectronInspectableWebContentsView;

namespace electron {

class InspectableWebContents;

class InspectableWebContentsViewMac : public InspectableWebContentsView {
 public:
  explicit InspectableWebContentsViewMac(
      InspectableWebContents* inspectable_web_contents);
  InspectableWebContentsViewMac(const InspectableWebContentsViewMac&) = delete;
  InspectableWebContentsViewMac& operator=(
      const InspectableWebContentsViewMac&) = delete;
  ~InspectableWebContentsViewMac() override;

  gfx::NativeView GetNativeView() const override;
  void ShowDevTools(bool activate) override;
  void CloseDevTools() override;
  bool IsDevToolsViewShowing() override;
  bool IsDevToolsViewFocused() override;
  void SetIsDocked(bool docked, bool activate) override;
  void SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) override;
  void SetTitle(const std::u16string& title) override;

  InspectableWebContents* inspectable_web_contents() {
    return inspectable_web_contents_;
  }

 private:
  // 拥有我们。
  InspectableWebContents* inspectable_web_contents_;

  base::scoped_nsobject<ElectronInspectableWebContentsView> view_;
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_
