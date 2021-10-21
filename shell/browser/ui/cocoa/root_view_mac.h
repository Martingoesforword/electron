// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_ROOT_VIEW_MAC_H_
#define SHELL_BROWSER_UI_COCOA_ROOT_VIEW_MAC_H_

#include "ui/views/view.h"

namespace electron {

class NativeWindow;

class RootViewMac : public views::View {
 public:
  explicit RootViewMac(NativeWindow* window);
  ~RootViewMac() override;

  // 视图：：视图：
  void Layout() override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;

 private:
  // 父窗口，弱引用。
  NativeWindow* window_;

  DISALLOW_COPY_AND_ASSIGN(RootViewMac);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_COCA_ROOT_VIEW_MAC_H_
