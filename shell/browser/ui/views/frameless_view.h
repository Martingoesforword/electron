// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_VIEWS_FRAMELESS_VIEW_H_
#define SHELL_BROWSER_UI_VIEWS_FRAMELESS_VIEW_H_

#include "ui/views/window/non_client_view.h"

namespace views {
class Widget;
}

namespace electron {

class NativeWindowViews;

class FramelessView : public views::NonClientFrameView {
 public:
  static const char kViewClassName[];
  FramelessView();
  ~FramelessView() override;

  virtual void Init(NativeWindowViews* window, views::Widget* frame);

  // 返回|point|是否在无框架窗口的调整边框上。
  int ResizingBorderHitTest(const gfx::Point& point);

 protected:
  // 视图：：NonClientFrameView：
  gfx::Rect GetBoundsForClientView() const override;
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void GetWindowMask(const gfx::Size& size, SkPath* window_mask) override;
  void ResetWindowControls() override;
  void UpdateWindowIcon() override;
  void UpdateWindowTitle() override;
  void SizeConstraintsChanged() override;

  // 从视图覆盖：
  gfx::Size CalculatePreferredSize() const override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  const char* GetClassName() const override;

  // 不是所有的。
  NativeWindowViews* window_ = nullptr;
  views::Widget* frame_ = nullptr;

  friend class NativeWindowsViews;

 private:
  DISALLOW_COPY_AND_ASSIGN(FramelessView);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Views_Framless_VIEW_H_
