// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_
#define SHELL_BROWSER_OSR_OSR_WEB_CONTENTS_VIEW_H_

#include "shell/browser/native_window.h"
#include "shell/browser/native_window_observer.h"

#include "content/browser/renderer_host/render_view_host_delegate_view.h"  // 点名检查。
#include "content/browser/web_contents/web_contents_view.h"  // 点名检查。
#include "content/public/browser/web_contents.h"
#include "shell/browser/osr/osr_render_widget_host_view.h"
#include "third_party/blink/public/common/page/drag_mojom_traits.h"

#if defined(OS_MAC)
#ifdef __OBJC__
@class OffScreenView;
#else
class OffScreenView;
#endif
#endif

namespace electron {

class OffScreenWebContentsView : public content::WebContentsView,
                                 public content::RenderViewHostDelegateView,
                                 public NativeWindowObserver {
 public:
  OffScreenWebContentsView(bool transparent, const OnPaintCallback& callback);
  ~OffScreenWebContentsView() override;

  void SetWebContents(content::WebContents*);
  void SetNativeWindow(NativeWindow* window);

  // NativeWindowViewer：
  void OnWindowResize() override;
  void OnWindowClosed() override;

  gfx::Size GetSize();

  // 内容：：WebContentsView：
  gfx::NativeView GetNativeView() const override;
  gfx::NativeView GetContentNativeView() const override;
  gfx::NativeWindow GetTopLevelNativeWindow() const override;
  gfx::Rect GetContainerBounds() const override;
  void Focus() override;
  void SetInitialFocus() override;
  void StoreFocus() override;
  void RestoreFocus() override;
  void FocusThroughTabTraversal(bool reverse) override;
  content::DropData* GetDropData() const override;
  gfx::Rect GetViewBounds() const override;
  void CreateView(gfx::NativeView context) override;
  content::RenderWidgetHostViewBase* CreateViewForWidget(
      content::RenderWidgetHost* render_widget_host) override;
  content::RenderWidgetHostViewBase* CreateViewForChildWidget(
      content::RenderWidgetHost* render_widget_host) override;
  void SetPageTitle(const std::u16string& title) override;
  void RenderViewReady() override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  void SetOverscrollControllerEnabled(bool enabled) override;

#if defined(OS_MAC)
  bool CloseTabAfterEventTrackingIfNeeded() override;
#endif

  // 内容：：RenderViewHostDelegateView。
  void StartDragging(const content::DropData& drop_data,
                     blink::DragOperationsMask allowed_ops,
                     const gfx::ImageSkia& image,
                     const gfx::Vector2d& image_offset,
                     const blink::mojom::DragEventSourceInfo& event_info,
                     content::RenderWidgetHostImpl* source_rwh) override;
  void UpdateDragCursor(ui::mojom::DragOperation operation) override;
  void SetPainting(bool painting);
  bool IsPainting() const;
  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;

 private:
#if defined(OS_MAC)
  void PlatformCreate();
  void PlatformDestroy();
#endif

  OffScreenRenderWidgetHostView* GetView() const;

  NativeWindow* native_window_ = nullptr;

  const bool transparent_;
  bool painting_ = true;
  int frame_rate_ = 60;
  OnPaintCallback callback_;

  // 软弱的裁判。
  content::WebContents* web_contents_ = nullptr;

#if defined(OS_MAC)
  OffScreenView* offScreenView_;
#endif
};

}  // 命名空间电子。

#endif  // Shell_Browser_OSR_OSR_Web_Contents_VIEW_H_
