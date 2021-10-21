// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。
// 
// 此文件的某些部分源自。
// Chrome/browser/ui/views/frame/glass_browser_frame_view.h，
// 版权所有(C)2012 Chromium作者，
// 它由BSD样式的许可证管理。

#ifndef SHELL_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_
#define SHELL_BROWSER_UI_VIEWS_WIN_FRAME_VIEW_H_

#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/views/frameless_view.h"
#include "shell/browser/ui/views/win_caption_button.h"

namespace electron {

class WinFrameView : public FramelessView {
 public:
  static const char kViewClassName[];
  WinFrameView();
  ~WinFrameView() override;

  void Init(NativeWindowViews* window, views::Widget* frame) override;

  // 用于标题栏中的要素(窗口标题和标题)的Alpha。
  // 按钮)，当窗口处于非活动状态时。它们在处于活动状态时是不透明的。
  static constexpr SkAlpha kInactiveTitlebarFeatureAlpha = 0x66;

  SkColor GetReadableFeatureColor(SkColor background_color);

  // 视图：：NonClientFrameView：
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;

  // 视图：：视图：
  const char* GetClassName() const override;

  NativeWindowViews* window() const { return window_; }
  views::Widget* frame() const { return frame_; }

  bool IsMaximized() const;

  bool ShouldCustomDrawSystemTitlebar() const;

  // 窗口最大化时标题栏的视觉高度(即不包括。
  // 屏幕顶部上方的区域)。
  int TitlebarMaximizedVisualHeight() const;

 protected:
  // 视图：：视图：
  void Layout() override;

 private:
  friend class WinCaptionButtonContainer;

  int FrameBorderThickness() const;

  // 返回框架上边缘的窗口边框厚度，
  // 它有时与FrameBorderThickness()不同。不包括。
  // 标题栏/选项卡栏区域。如果|RESTORED|为TRUE，则按如下方式计算。
  // 无论窗口的当前状态如何，窗口都已恢复。
  int FrameTopBorderThickness(bool restored) const;
  int FrameTopBorderThicknessPx(bool restored) const;

  // 返回弹出窗口或其他浏览器类型的标题栏高度，
  // 没有标签。
  int TitlebarHeight(bool restored) const;

  // 返回框架顶部的y坐标，该坐标处于最大化模式。
  // 是屏幕的顶部，在还原模式下是比。
  // 为Windows绘制的可视边框留出空间的窗口。
  int WindowTopY() const;

  void LayoutCaptionButtons();
  void LayoutWindowControlsOverlay();

  // 容纳标题按钮(最小化、最大化、关闭等)的容器。
  // 如果字幕按钮容器在帧之前被销毁，则可能为空。
  // 查看。在使用之前一定要检查有效性！
  WinCaptionButtonContainer* caption_button_container_;

  DISALLOW_COPY_AND_ASSIGN(WinFrameView);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Views_Win_Frame_View_H_
