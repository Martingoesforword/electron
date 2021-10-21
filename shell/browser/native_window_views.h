// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NATIVE_WINDOW_VIEWS_H_
#define SHELL_BROWSER_NATIVE_WINDOW_VIEWS_H_

#include "shell/browser/native_window.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "shell/common/api/api.mojom.h"
#include "ui/views/widget/widget_observer.h"

#if defined(OS_WIN)
#include "base/win/scoped_gdi_object.h"
#include "shell/browser/ui/win/taskbar_host.h"

#endif

namespace views {
class UnhandledKeyboardEventHandler;
}

namespace electron {

class GlobalMenuBarX11;
class RootView;
class WindowStateWatcher;

#if defined(USE_X11)
class EventDisabler;
#endif

#if defined(OS_WIN)
gfx::Rect ScreenToDIPRect(HWND hwnd, const gfx::Rect& pixel_bounds);
#endif

class NativeWindowViews : public NativeWindow,
                          public views::WidgetObserver,
                          public ui::EventHandler {
 public:
  NativeWindowViews(const gin_helper::Dictionary& options,
                    NativeWindow* parent);
  ~NativeWindowViews() override;

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
  void SetBounds(const gfx::Rect& bounds, bool animate) override;
  gfx::Rect GetBounds() override;
  gfx::Rect GetContentBounds() override;
  gfx::Size GetContentSize() override;
  gfx::Rect GetNormalBounds() override;
  SkColor GetBackgroundColor() override;
  void SetContentSizeConstraints(
      const extensions::SizeConstraints& size_constraints) override;
  void SetResizable(bool resizable) override;
  bool MoveAbove(const std::string& sourceId) override;
  void MoveTop() override;
  bool IsResizable() override;
  void SetAspectRatio(double aspect_ratio,
                      const gfx::Size& extra_size) override;
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
                      int relativeLevel) override;
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
  bool IsTabletMode() const override;
  void SetBackgroundColor(SkColor color) override;
  void SetHasShadow(bool has_shadow) override;
  bool HasShadow() override;
  void SetOpacity(const double opacity) override;
  double GetOpacity() override;
  void SetIgnoreMouseEvents(bool ignore, bool forward) override;
  void SetContentProtection(bool enable) override;
  void SetFocusable(bool focusable) override;
  bool IsFocusable() override;
  void SetMenu(ElectronMenuModel* menu_model) override;
  void AddBrowserView(NativeBrowserView* browser_view) override;
  void RemoveBrowserView(NativeBrowserView* browser_view) override;
  void SetTopBrowserView(NativeBrowserView* browser_view) override;
  void SetParentWindow(NativeWindow* parent) override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  void SetOverlayIcon(const gfx::Image& overlay,
                      const std::string& description) override;
  void SetProgressBar(double progress, const ProgressState state) override;
  void SetAutoHideMenuBar(bool auto_hide) override;
  bool IsMenuBarAutoHide() override;
  void SetMenuBarVisibility(bool visible) override;
  bool IsMenuBarVisible() override;

  void SetVisibleOnAllWorkspaces(bool visible,
                                 bool visibleOnFullScreen,
                                 bool skipTransformProcessType) override;

  bool IsVisibleOnAllWorkspaces() override;

  void SetGTKDarkThemeEnabled(bool use_dark_theme) override;

  content::DesktopMediaID GetDesktopMediaID() const override;
  gfx::AcceleratedWidget GetAcceleratedWidget() const override;
  NativeWindowHandle GetNativeWindowHandle() const override;

  gfx::Rect ContentBoundsToWindowBounds(const gfx::Rect& bounds) const override;
  gfx::Rect WindowBoundsToContentBounds(const gfx::Rect& bounds) const override;

  void UpdateDraggableRegions(
      const std::vector<mojom::DraggableRegionPtr>& regions);

  void IncrementChildModals();
  void DecrementChildModals();

#if defined(OS_WIN)
  // 包罗万象的消息处理和过滤。之前调用过。
  // HWNDMessageHandler的内置处理，这可能会先发制人。
  // 如果邮件被消费，视图/光环中的期望值。如果设置为True，则返回True。
  // 消息已由委托使用，不应进一步处理。
  // 由HWNDMessageHandler执行。在本例中，返回|Result|。|Result|为。
  // 未以其他方式修改。
  bool PreHandleMSG(UINT message,
                    WPARAM w_param,
                    LPARAM l_param,
                    LRESULT* result);
  void SetIcon(HICON small_icon, HICON app_icon);
#elif defined(OS_LINUX)
  void SetIcon(const gfx::ImageSkia& icon);
#endif

  SkRegion* draggable_region() const { return draggable_region_.get(); }

#if defined(OS_WIN)
  TaskbarHost& taskbar_host() { return taskbar_host_; }
#endif

#if defined(OS_WIN)
  bool IsWindowControlsOverlayEnabled() const {
    return (title_bar_style_ == NativeWindowViews::TitleBarStyle::kHidden) &&
           titlebar_overlay_;
  }
  SkColor overlay_button_color() const { return overlay_button_color_; }
  SkColor overlay_symbol_color() const { return overlay_symbol_color_; }
#endif

 private:
  // 视图：：WidgetViewer：
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& bounds) override;
  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetDestroyed(views::Widget* widget) override;

  // 视图：：WidgetDelegate：
  views::View* GetInitiallyFocusedView() override;
  bool CanMaximize() const override;
  bool CanMinimize() const override;
  std::u16string GetWindowTitle() const override;
  views::View* GetContentsView() override;
  bool ShouldDescendIntoChildForEventHandling(
      gfx::NativeView child,
      const gfx::Point& location) override;
  views::ClientView* CreateClientView(views::Widget* widget) override;
  std::unique_ptr<views::NonClientFrameView> CreateNonClientFrameView(
      views::Widget* widget) override;
  void OnWidgetMove() override;
#if defined(OS_WIN)
  bool ExecuteWindowsCommand(int command_id) override;
#endif

#if defined(OS_WIN)
  void HandleSizeEvent(WPARAM w_param, LPARAM l_param);
  void SetForwardMouseMessages(bool forward);
  static LRESULT CALLBACK SubclassProc(HWND hwnd,
                                       UINT msg,
                                       WPARAM w_param,
                                       LPARAM l_param,
                                       UINT_PTR subclass_id,
                                       DWORD_PTR ref_data);
  static LRESULT CALLBACK MouseHookProc(int n_code,
                                        WPARAM w_param,
                                        LPARAM l_param);
#endif

  // 启用/禁用：
  bool ShouldBeEnabled();
  void SetEnabledInternal(bool enabled);

  // NativeWindow：
  void HandleKeyboardEvent(
      content::WebContents*,
      const content::NativeWebKeyboardEvent& event) override;

  // UI：：EventHandler：
  void OnMouseEvent(ui::MouseEvent* event) override;

  // 返回窗口的还原状态。
  ui::WindowShowState GetRestoredState();

  // 保持窗的位置。
  void MoveBehindTaskBarIfNeeded();

  std::unique_ptr<RootView> root_view_;

  // 默认情况下，视图应该是聚焦的。
  views::View* focused_view_ = nullptr;

  // Linux上的“可调整大小”标志是通过设置大小约束来实现的，
  // 我们需要确保在窗口变为。
  // 再次调整大小。这也可以在Windows上使用，以保持任务栏的大小调整。
  // 调整窗口大小引发的事件。
  extensions::SizeConstraints old_size_constraints_;

#if defined(USE_X11)
  std::unique_ptr<GlobalMenuBarX11> global_menu_bar_;

  // 处理窗口状态事件。
  std::unique_ptr<WindowStateWatcher> window_state_watcher_;

  // 要禁用鼠标事件，请执行以下操作。
  std::unique_ptr<EventDisabler> event_disabler_;
#endif

#if defined(OS_WIN)

  ui::WindowShowState last_window_state_;

  gfx::Rect last_normal_placement_bounds_;

  // 负责任务栏相关API的运行。
  TaskbarHost taskbar_host_;

  // A11y支票的记忆版本。
  bool checked_for_a11y_support_ = false;

  // 是否显示WS_THICKFRAME样式。
  bool thick_frame_ = true;

  // 最大化/全屏之前的窗口边界。
  gfx::Rect restore_bounds_;

  // 窗口和任务栏的图标。
  base::win::ScopedHICON window_icon_;
  base::win::ScopedHICON app_icon_;

  // 当前转发鼠标消息的窗口集。
  static std::set<NativeWindowViews*> forwarding_windows_;
  static HHOOK mouse_hook_;
  bool forwarding_mouse_messages_ = false;
  HWND legacy_window_ = NULL;
  bool layered_ = false;

  // 如果窗口始终位于任务栏的顶部和后面，则设置为true。
  bool behind_task_bar_ = false;

  // 是否要设置没有副作用的窗口位置。
  bool is_setting_window_placement_ = false;

  // 窗口当前是否正在调整大小。
  bool is_resizing_ = false;

  // 窗口当前是否正在移动。
  bool is_moving_ = false;

  // 分别用作窗口的主题颜色和符号颜色的颜色。
  // 控件覆盖(如果在Windows上启用)。
  SkColor overlay_button_color_;
  SkColor overlay_symbol_color_;
#endif

  // 处理从呈现器进程返回的未处理键盘消息。
  std::unique_ptr<views::UnhandledKeyboardEventHandler> keyboard_event_handler_;

  // 对于自定义拖动，整个窗口是不可拖动的，并且可拖动区域。
  // 必须明确规定。
  std::unique_ptr<SkRegion> draggable_region_;  // 在自定义阻力中使用。

  // 是否应根据用户对SetEnabled()的调用启用窗口。
  bool is_enabled_ = true;
  // 此窗口有多少模式子窗口；
  // 用于确定启用状态。
  unsigned int num_modal_children_ = 0;

  bool use_content_size_ = false;
  bool movable_ = true;
  bool resizable_ = true;
  bool maximizable_ = true;
  bool minimizable_ = true;
  bool fullscreenable_ = true;
  std::string title_;
  gfx::Size widget_size_;
  double opacity_ = 1.0;
  bool widget_destroyed_ = false;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowViews);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Native_Window_Views_H_
