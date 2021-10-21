// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_NATIVE_FRAME_VIEW_H_
#define SHELL_BROWSER_UI_VIEWS_NATIVE_FRAME_VIEW_H_

#include "ui/views/window/native_frame_view.h"

namespace electron {

class NativeWindow;

// 类似于视图：：NativeFrameView，但返回。
// NativeWindowViews。
class NativeFrameView : public views::NativeFrameView {
 public:
  static const char kViewClassName[];
  NativeFrameView(NativeWindow* window, views::Widget* widget);

 protected:
  // 视图：：视图：
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  const char* GetClassName() const override;

 private:
  NativeWindow* window_;  // 弱小的裁判。

  DISALLOW_COPY_AND_ASSIGN(NativeFrameView);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Views_Native_Frame_View_H_
