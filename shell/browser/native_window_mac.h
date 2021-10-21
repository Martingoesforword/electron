// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NATIVE_WINDOW_MAC_H_
#define SHELL_BROWSER_NATIVE_WINDOW_MAC_H_

#import <Cocoa/Cocoa.h>

#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "base/mac/scoped_nsobject.h"
#include "shell/browser/native_window.h"
#include "ui/display/display_observer.h"
#include "ui/native_theme/native_theme_observer.h"
#include "ui/views/controls/native/native_view_host.h"

@class ElectronNSWindow;
@class ElectronNSWindowDelegate;
@class ElectronPreviewItem;
@class ElectronTouchBar;
@class WindowButtonsProxy;

namespace electron {

class RootViewMac;

class NativeWindowMac : public NativeWindow,
                        public ui::NativeThemeObserver,
                        public display::DisplayObserver {
 public:
  NativeWindowMac(const gin_helper::Dictionary& options, NativeWindow* parent);
  ~NativeWindowMac() override;

  // NativeWindow：
  void SetContentView(views::View* view) override;
  void Close() override;
  void CloseImmediately() override;
  void Focus(bool focus) override;
  bool IsFocused() override;
  void Show() override;
  void ShowInactive() override;
  void Hide() override;
  bool IsVisible() override;
  bool IsEnabled() override;
  void SetEnabled(bool enable) override;
  void Maximize() override;
  void Unmaximize() override;
  bool IsMaximized() override;
  void Minimize() override;
  void Restore() override;
  bool IsMinimized() override;
  void SetFullScreen(bool fullscreen) override;
  bool IsFullscreen() const override;
  void SetBounds(const gfx::Rect& bounds, bool animate = false) override;
  gfx::Rect GetBounds() override;
  bool IsNormal() override;
  gfx::Rect GetNormalBounds() override;
  void SetContentSizeConstraints(
      const extensions::SizeConstraints& size_constraints) override;
  void SetResizable(bool resizable) override;
  bool MoveAbove(const std::string& sourceId) override;
  void MoveTop() override;
  bool IsResizable() override;
  void SetMovable(bool movable) override;
  bool IsMovable() override;
  void SetMinimizable(bool minimizable) override;
  bool IsMinimizable() override;
  void SetMaximizable(bool maximizable) override;
  bool IsMaximizable() override;
  void SetFullScreenable(bool fullscreenable) override;
  bool IsFullScreenable() override;
  void SetClosable(bool closable) override;
  bool IsClosable() override;
  void SetAlwaysOnTop(ui::ZOrderLevel z_order,
                      const std::string& level,
                      int relative_level) override;
  std::string GetAlwaysOnTopLevel() override;
  ui::ZOrderLevel GetZOrderLevel() override;
  void Center() override;
  void Invalidate() override;
  void SetTitle(const std::string& title) override;
  std::string GetTitle() override;
  void FlashFrame(bool flash) override;
  void SetSkipTaskbar(bool skip) override;
  void SetExcludedFromShownWindowsMenu(bool excluded) override;
  bool IsExcludedFromShownWindowsMenu() override;
  void SetSimpleFullScreen(bool simple_fullscreen) override;
  bool IsSimpleFullScreen() override;
  void SetKiosk(bool kiosk) override;
  bool IsKiosk() override;
  void SetBackgroundColor(SkColor color) override;
  SkColor GetBackgroundColor() override;
  void SetHasShadow(bool has_shadow) override;
  bool HasShadow() override;
  void SetOpacity(const double opacity) override;
  double GetOpacity() override;
  void SetRepresentedFilename(const std::string& filename) override;
  std::string GetRepresentedFilename() override;
  void SetDocumentEdited(bool edited) override;
  bool IsDocumentEdited() override;
  void SetIgnoreMouseEvents(bool ignore, bool forward) override;
  void SetContentProtection(bool enable) override;
  void SetFocusable(bool focusable) override;
  bool IsFocusable() override;
  void AddBrowserView(NativeBrowserView* browser_view) override;
  void RemoveBrowserView(NativeBrowserView* browser_view) override;
  void SetTopBrowserView(NativeBrowserView* browser_view) override;
  void SetParentWindow(NativeWindow* parent) override;
  content::DesktopMediaID GetDesktopMediaID() const override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  gfx::AcceleratedWidget GetAcceleratedWidget() const override;
  NativeWindowHandle GetNativeWindowHandle() const override;
  void SetProgressBar(double progress, const ProgressState state) override;
  void SetOverlayIcon(const gfx::Image& overlay,
                      const std::string& description) override;
  void SetVisibleOnAllWorkspaces(bool visible,
                                 bool visibleOnFullScreen,
                                 bool skipTransformProcessType) override;
  bool IsVisibleOnAllWorkspaces() override;
  void SetAutoHideCursor(bool auto_hide) override;
  void SetVibrancy(const std::string& type) override;
  void SetWindowButtonVisibility(bool visible) override;
  bool GetWindowButtonVisibility() const override;
  void SetTrafficLightPosition(absl::optional<gfx::Point> position) override;
  absl::optional<gfx::Point> GetTrafficLightPosition() const override;
  void RedrawTrafficLights() override;
  void UpdateFrame() override;
  void SetTouchBar(
      std::vector<gin_helper::PersistentDictionary> items) override;
  void RefreshTouchBarItem(const std::string& item_id) override;
  void SetEscapeTouchBarItem(gin_helper::PersistentDictionary item) override;
  void SelectPreviousTab() override;
  void SelectNextTab() override;
  void MergeAllWindows() override;
  void MoveTabToNewWindow() override;
  void ToggleTabBar() override;
  bool AddTabbedWindow(NativeWindow* window) override;
  void SetAspectRatio(double aspect_ratio,
                      const gfx::Size& extra_size) override;
  void PreviewFile(const std::string& path,
                   const std::string& display_name) override;
  void CloseFilePreview() override;
  gfx::Rect ContentBoundsToWindowBounds(const gfx::Rect& bounds) const override;
  gfx::Rect WindowBoundsToContentBounds(const gfx::Rect& bounds) const override;
  gfx::Rect GetWindowControlsOverlayRect() override;
  void NotifyWindowEnterFullScreen() override;
  void NotifyWindowLeaveFullScreen() override;
  void SetActive(bool is_key) override;
  bool IsActive() const override;

  void NotifyWindowWillEnterFullScreen();
  void NotifyWindowWillLeaveFullScreen();

  // 窗口关闭时清理观察器。请注意，析构函数。
  // 可以在窗口关闭后很久才调用，所以我们不应该这样做。
  // 析构函数中的清除。
  void Cleanup();

  // 使用自定义内容视图，而不是Chromium的BridgedContentView。
  void OverrideNSWindowContentView();

  void UpdateVibrancyRadii(bool fullscreen);

  // 设置NSWindow的属性，同时解决缩放按钮的错误。
  void SetStyleMask(bool on, NSUInteger flag);
  void SetCollectionBehavior(bool on, NSUInteger flag);
  void SetWindowLevel(int level);

  enum class FullScreenTransitionState { ENTERING, EXITING, NONE };

  // 处理全屏过渡。
  void SetFullScreenTransitionState(FullScreenTransitionState state);
  void HandlePendingFullscreenTransitions();

  enum class VisualEffectState {
    kFollowWindow,
    kActive,
    kInactive,
  };

  ElectronPreviewItem* preview_item() const { return preview_item_.get(); }
  ElectronTouchBar* touch_bar() const { return touch_bar_.get(); }
  bool zoom_to_page_width() const { return zoom_to_page_width_; }
  bool always_simple_fullscreen() const { return always_simple_fullscreen_; }

  // 我们需要保存windowWillUseStandardFrame：defaultFrame的结果。
  // 因为MacOS用它所说的“最适合”的框架来称呼它。
  // 缩放。这意味着即使设置了纵横比，MacOS也可能会调整它。
  // 以便更好地适应屏幕。
  // 
  // 因此，我们不能只计算最大纵横比的大小。
  // 当前可见屏幕，并将其与当前窗口的框架进行比较。
  // 若要确定窗口是否最大化，请执行以下操作。
  NSRect default_frame_for_zoom() const { return default_frame_for_zoom_; }
  void set_default_frame_for_zoom(NSRect frame) {
    default_frame_for_zoom_ = frame;
  }

 protected:
  // 视图：：WidgetDelegate：
  views::View* GetContentsView() override;
  bool CanMaximize() const override;

  // UI：：NativeThemeViewer：
  void OnNativeThemeUpdated(ui::NativeTheme* observed_theme) override;

  // Display：：DisplayViewer：
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t changed_metrics) override;

 private:
  // 将自定义层添加到内容视图。
  void AddContentViewLayers();

  void InternalSetWindowButtonVisibility(bool visible);
  void InternalSetParentWindow(NativeWindow* parent, bool attach);
  void SetForwardMouseMessages(bool forward);

  ElectronNSWindow* window_;  // 弱引用，由Widget_管理。

  base::scoped_nsobject<ElectronNSWindowDelegate> window_delegate_;
  base::scoped_nsobject<ElectronPreviewItem> preview_item_;
  base::scoped_nsobject<ElectronTouchBar> touch_bar_;

  // 滚轮事件的事件监视器。
  id wheel_event_monitor_;

  // 用作Windows Content View的NSView。
  // 
  // 对于无边框窗口，它会填满整个窗口。
  base::scoped_nsobject<NSView> container_view_;

  // 填充工作区的Views：：View。
  std::unique_ptr<RootViewMac> root_view_;

  bool is_kiosk_ = false;
  bool zoom_to_page_width_ = false;
  absl::optional<gfx::Point> traffic_light_position_;

  std::queue<bool> pending_transitions_;
  FullScreenTransitionState fullscreen_transition_state() const {
    return fullscreen_transition_state_;
  }
  FullScreenTransitionState fullscreen_transition_state_ =
      FullScreenTransitionState::NONE;

  NSInteger attention_request_id_ = 0;  // 来自requestUserAttent的标识符。

  // 进入Kiosk模式之前的演示选项。
  NSApplicationPresentationOptions kiosk_options_;

  // “viewalEffectState”选项。
  VisualEffectState visual_effect_state_ = VisualEffectState::kFollowWindow;

  // 通过显式设置时，窗口按钮控件的可见性模式。
  // SetWindowButtonVisibility()。
  absl::optional<bool> window_button_visibility_;

  // 控制窗口按钮的位置和可见性。
  base::scoped_nsobject<WindowButtonsProxy> buttons_proxy_;

  // 最大窗口状态；在重绘过程中保持不变是必需的。
  bool maximizable_ = true;

  // 简单(Pre-Lion)全屏设置。
  bool always_simple_fullscreen_ = false;
  bool is_simple_fullscreen_ = false;
  bool was_maximizable_ = false;
  bool was_movable_ = false;
  bool is_active_ = false;
  NSRect original_frame_;
  NSInteger original_level_;
  NSUInteger simple_fullscreen_mask_;
  NSRect default_frame_for_zoom_;

  std::string vibrancy_type_;

  // 进入简单全屏模式之前的演示选项。
  NSApplicationPresentationOptions simple_fullscreen_options_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowMac);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Native_Window_MAC_H_
