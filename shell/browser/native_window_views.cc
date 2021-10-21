// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/native_window_views.h"

#if defined(OS_WIN)
#include <wrl/client.h>
#endif

#include <memory>
#include <utility>
#include <vector>

#include "base/cxx17_backports.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/desktop_media_id.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/native_browser_view_views.h"
#include "shell/browser/ui/drag_util.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/browser/ui/views/root_view.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/browser/web_view_manager.h"
#include "shell/browser/window_list.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/options_switches.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/background.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/native_widget_private.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/client_view.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/window_util.h"

#if defined(OS_LINUX)
#include "base/strings/string_util.h"
#include "shell/browser/browser.h"
#include "shell/browser/linux/unity_service.h"
#include "shell/browser/ui/views/frameless_view.h"
#include "shell/browser/ui/views/native_frame_view.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_linux.h"
#include "ui/views/window/native_frame_view.h"

#if defined(USE_X11)
#include "shell/browser/ui/views/global_menu_bar_x11.h"
#include "shell/browser/ui/x/event_disabler.h"
#include "shell/browser/ui/x/window_state_watcher.h"
#include "shell/browser/ui/x/x_window_utils.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/x/shape.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/gfx/x/xproto.h"
#include "ui/gfx/x/xproto_util.h"
#endif

#if defined(USE_OZONE) || defined(USE_X11)
#include "ui/base/ui_base_features.h"
#endif

#elif defined(OS_WIN)
#include "base/win/win_util.h"
#include "extensions/common/image_util.h"
#include "shell/browser/ui/views/win_frame_view.h"
#include "shell/browser/ui/win/electron_desktop_native_widget_aura.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/base/win/shell.h"
#include "ui/display/screen.h"
#include "ui/display/win/screen_win.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

namespace electron {

#if defined(OS_WIN)

// 类似于Display：：Win：：ScreenWin中的值，但具有四舍五入的值。
// 这些功能有助于避免因不可调整大小的窗口而引起的问题，在不可调整窗口中，
// 原始ceil()值可能会导致计算错误，因为。
// 这两种方法都通过ceil()调用。相关问题：#15816。
gfx::Rect ScreenToDIPRect(HWND hwnd, const gfx::Rect& pixel_bounds) {
  float scale_factor = display::win::ScreenWin::GetScaleFactorForHWND(hwnd);
  gfx::Rect dip_rect = ScaleToRoundedRect(pixel_bounds, 1.0f / scale_factor);
  dip_rect.set_origin(
      display::win::ScreenWin::ScreenToDIPRect(hwnd, pixel_bounds).origin());
  return dip_rect;
}

#endif

namespace {

#if defined(OS_WIN)
const LPCWSTR kUniqueTaskBarClassName = L"Shell_TrayWnd";

void FlipWindowStyle(HWND handle, bool on, DWORD flag) {
  DWORD style = ::GetWindowLong(handle, GWL_STYLE);
  if (on)
    style |= flag;
  else
    style &= ~flag;
  ::SetWindowLong(handle, GWL_STYLE, style);
  // 窗口的框架样式已缓存，因此需要调用SetWindowPos。
  // 使用SWP_FRAMECHANGED标志正确更新缓存。
  ::SetWindowPos(handle, 0, 0, 0, 0, 0,  // 忽略。
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

gfx::Rect DIPToScreenRect(HWND hwnd, const gfx::Rect& pixel_bounds) {
  float scale_factor = display::win::ScreenWin::GetScaleFactorForHWND(hwnd);
  gfx::Rect screen_rect = ScaleToRoundedRect(pixel_bounds, scale_factor);
  screen_rect.set_origin(
      display::win::ScreenWin::DIPToScreenRect(hwnd, pixel_bounds).origin());
  return screen_rect;
}

#endif

class NativeWindowClientView : public views::ClientView {
 public:
  NativeWindowClientView(views::Widget* widget,
                         views::View* root_view,
                         NativeWindowViews* window)
      : views::ClientView(widget, root_view), window_(window) {}
  ~NativeWindowClientView() override = default;

  views::CloseRequestResult OnWindowCloseRequested() override {
    window_->NotifyWindowCloseButtonClicked();
    return views::CloseRequestResult::kCannotClose;
  }

 private:
  NativeWindowViews* window_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowClientView);
};

}  // 命名空间。

NativeWindowViews::NativeWindowViews(const gin_helper::Dictionary& options,
                                     NativeWindow* parent)
    : NativeWindow(options, parent),
      root_view_(std::make_unique<RootView>(this)),
      keyboard_event_handler_(
          std::make_unique<views::UnhandledKeyboardEventHandler>()) {
  options.Get(options::kTitle, &title_);

  bool menu_bar_autohide;
  if (options.Get(options::kAutoHideMenuBar, &menu_bar_autohide))
    root_view_->SetAutoHideMenuBar(menu_bar_autohide);

#if defined(OS_WIN)
  // 在Windows上，我们依赖CanResize()来指示窗口是否可以。
  // 已调整大小，应在创建窗口之前设置。
  options.Get(options::kResizable, &resizable_);
  options.Get(options::kMinimizable, &minimizable_);
  options.Get(options::kMaximizable, &maximizable_);

  // 透明窗不能有厚边框。
  options.Get("thickFrame", &thick_frame_);
  if (transparent())
    thick_frame_ = false;

  overlay_button_color_ = color_utils::GetSysSkColor(COLOR_BTNFACE);
  overlay_symbol_color_ = color_utils::GetSysSkColor(COLOR_BTNTEXT);

  v8::Local<v8::Value> titlebar_overlay;
  if (options.Get(options::ktitleBarOverlay, &titlebar_overlay) &&
      titlebar_overlay->IsObject()) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    gin_helper::Dictionary titlebar_overlay_obj =
        gin::Dictionary::CreateEmpty(isolate);
    options.Get(options::ktitleBarOverlay, &titlebar_overlay_obj);

    std::string overlay_color_string;
    if (titlebar_overlay_obj.Get(options::kOverlayButtonColor,
                                 &overlay_color_string)) {
      bool success = extensions::image_util::ParseCssColorString(
          overlay_color_string, &overlay_button_color_);
      DCHECK(success);
    }

    std::string overlay_symbol_color_string;
    if (titlebar_overlay_obj.Get(options::kOverlaySymbolColor,
                                 &overlay_symbol_color_string)) {
      bool success = extensions::image_util::ParseCssColorString(
          overlay_symbol_color_string, &overlay_symbol_color_);
      DCHECK(success);
    }
  }

  if (title_bar_style_ != TitleBarStyle::kNormal)
    set_has_frame(false);
#endif

  if (enable_larger_than_screen())
    // 我们需要在此处设置默认的最大窗口大小，否则Windows。
    // 将不允许我们调整大于屏幕的窗口大小。
    // 不知何故，直接设置为INT_MAX不起作用，所以我们只是除以。
    // 到10点，应该还是够大的。
    SetContentSizeConstraints(extensions::SizeConstraints(
        gfx::Size(), gfx::Size(INT_MAX / 10, INT_MAX / 10)));

  int width = 800, height = 600;
  options.Get(options::kWidth, &width);
  options.Get(options::kHeight, &height);
  gfx::Rect bounds(0, 0, width, height);
  widget_size_ = bounds.size();

  widget()->AddObserver(this);

  views::Widget::InitParams params;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.bounds = bounds;
  params.delegate = this;
  params.type = views::Widget::InitParams::TYPE_WINDOW;
  params.remove_standard_frame = !has_frame();

  if (transparent())
    params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;

  // 给定的窗口很可能不是矩形的，因为它使用。
  // 透明且没有标准边框，请不要为其显示阴影。
  if (transparent() && !has_frame())
    params.shadow_type = views::Widget::InitParams::ShadowType::kNone;

  bool focusable;
  if (options.Get(options::kFocusable, &focusable) && !focusable)
    params.activatable = views::Widget::InitParams::Activatable::kNo;

#if defined(OS_WIN)
  if (parent)
    params.parent = parent->GetNativeWindow();

  params.native_widget = new ElectronDesktopNativeWidgetAura(this);
#elif defined(OS_LINUX)
  std::string name = Browser::Get()->GetName();
  // 设置WM_WINDOW_ROLE。
  params.wm_role_name = "browser-window";
  // 设置WM_CLASS。
  params.wm_class_name = base::ToLowerASCII(name);
  params.wm_class_class = name;
#endif

  widget()->Init(std::move(params));
  SetCanResize(resizable_);

  bool fullscreen = false;
  options.Get(options::kFullscreen, &fullscreen);

  std::string window_type;
  options.Get(options::kType, &window_type);

#if defined(USE_X11)
  // 开始监视窗口状态。
  window_state_watcher_ = std::make_unique<WindowStateWatcher>(this);

  // 如果我们设置了“Dark-Theme”选项，则将_GTK_THEME_VARIANT设置为DULK。
  bool use_dark_theme = false;
  if (options.Get(options::kDarkTheme, &use_dark_theme) && use_dark_theme) {
    SetGTKDarkThemeEnabled(use_dark_theme);
  }
#endif

#if defined(OS_LINUX)
  if (parent)
    SetParentWindow(parent);
#endif

#if defined(USE_X11)
  // 在映射窗口之前，SetWMSpecState无法工作，因此我们需要。
  // 要手动设置_NET_WM_STATE，请执行以下操作。
  std::vector<x11::Atom> state_atom_list;
  bool skip_taskbar = false;
  if (options.Get(options::kSkipTaskbar, &skip_taskbar) && skip_taskbar) {
    state_atom_list.push_back(x11::GetAtom("_NET_WM_STATE_SKIP_TASKBAR"));
  }

  // 在映射窗口之前，没有show_FullScreen_STATE。
  if (fullscreen) {
    state_atom_list.push_back(x11::GetAtom("_NET_WM_STATE_FULLSCREEN"));
  }

  if (parent) {
    // 强制对子窗口使用对话框类型。
    window_type = "dialog";

    // 模式窗口需要_NET_WM_STATE_MODEL提示。
    if (is_modal())
      state_atom_list.push_back(x11::GetAtom("_NET_WM_STATE_MODAL"));
  }

  if (!state_atom_list.empty())
    SetArrayProperty(static_cast<x11::Window>(GetAcceleratedWidget()),
                     x11::GetAtom("_NET_WM_STATE"), x11::Atom::ATOM,
                     state_atom_list);

  // 设置_NET_WM_WINDOW_TYPE。
  if (!window_type.empty())
    SetWindowType(static_cast<x11::Window>(GetAcceleratedWidget()),
                  window_type);
#endif

#if defined(OS_WIN)
  if (!has_frame()) {
    // 设置窗口样式，以便在以下情况下获得最小化和最大化动画。
    // 无框架的。
    DWORD frame_style = WS_CAPTION | WS_OVERLAPPED;
    if (resizable_)
      frame_style |= WS_THICKFRAME;
    if (minimizable_)
      frame_style |= WS_MINIMIZEBOX;
    if (maximizable_)
      frame_style |= WS_MAXIMIZEBOX;
    // 我们不应该显示透明窗口的框架。
    if (!thick_frame_)
      frame_style &= ~(WS_THICKFRAME | WS_CAPTION);
    ::SetWindowLong(GetAcceleratedWidget(), GWL_STYLE, frame_style);
  }

  LONG ex_style = ::GetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE);
  if (window_type == "toolbar")
    ex_style |= WS_EX_TOOLWINDOW;
  ::SetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE, ex_style);
#endif

  if (has_frame()) {
    // TODO(Zcbenz)：这用于在Windows 2003上强制使用本机框架，
    // 我们应该检查在InitParams中设置它是否可行。
    widget()->set_frame_type(views::Widget::FrameType::kForceNative);
    widget()->FrameTypeChanged();
#if defined(OS_WIN)
    // 加厚框也适用于普通窗口。
    if (!thick_frame_)
      FlipWindowStyle(GetAcceleratedWidget(), false, WS_THICKFRAME);
#endif
  }

  // 默认内容视图。
  SetContentView(new views::View());

  gfx::Size size = bounds.size();
  if (has_frame() &&
      options.Get(options::kUseContentSize, &use_content_size_) &&
      use_content_size_)
    size = ContentBoundsToWindowBounds(gfx::Rect(size)).size();

  widget()->CenterWindow(size);

#if defined(OS_WIN)
  // 保存初始窗口状态。
  if (fullscreen)
    last_window_state_ = ui::SHOW_STATE_FULLSCREEN;
  else
    last_window_state_ = ui::SHOW_STATE_NORMAL;
#endif

  // 收听鼠标事件。
  aura::Window* window = GetNativeWindow();
  if (window)
    window->AddPreTargetHandler(this);

#if defined(OS_LINUX)
  // 在Linux上，在小部件初始化之后，我们可能必须强制设置
  // 如果边界小于当前显示，则为边界。
  SetBounds(gfx::Rect(GetPosition(), bounds.size()), false);
#endif

  SetOwnedByWidget(false);
  RegisterDeleteDelegateCallback(base::BindOnce(
      [](NativeWindowViews* window) {
        if (window->is_modal() && window->parent()) {
          auto* parent = window->parent();
          // 在当前窗口关闭后启用父窗口。
          static_cast<NativeWindowViews*>(parent)->DecrementChildModals();
          // 将焦点放在父窗口上。
          parent->Focus(true);
        }

        window->NotifyWindowClosed();
      },
      this));
}

NativeWindowViews::~NativeWindowViews() {
  widget()->RemoveObserver(this);

#if defined(OS_WIN)
  // 如果保留任何资源，请禁用鼠标转发以释放资源。
  SetForwardMouseMessages(false);
#endif

  aura::Window* window = GetNativeWindow();
  if (window)
    window->RemovePreTargetHandler(this);
}

void NativeWindowViews::SetGTKDarkThemeEnabled(bool use_dark_theme) {
#if defined(USE_X11)
  const std::string color = use_dark_theme ? "dark" : "light";
  x11::SetStringProperty(static_cast<x11::Window>(GetAcceleratedWidget()),
                         x11::GetAtom("_GTK_THEME_VARIANT"),
                         x11::GetAtom("UTF8_STRING"), color);
#endif
}

void NativeWindowViews::SetContentView(views::View* view) {
  if (content_view()) {
    root_view_->RemoveChildView(content_view());
  }
  set_content_view(view);
  focused_view_ = view;
  root_view_->AddChildView(content_view());
  root_view_->Layout();
}

void NativeWindowViews::Close() {
  if (!IsClosable()) {
    WindowList::WindowCloseCancelled(this);
    return;
  }

  widget()->Close();
}

void NativeWindowViews::CloseImmediately() {
  widget()->CloseNow();
}

void NativeWindowViews::Focus(bool focus) {
  // 对于隐藏窗口，Focus()不应执行任何操作。
  if (!IsVisible())
    return;

  if (focus) {
    widget()->Activate();
  } else {
    widget()->Deactivate();
  }
}

bool NativeWindowViews::IsFocused() {
  return widget()->IsActive();
}

void NativeWindowViews::Show() {
  if (is_modal() && NativeWindow::parent() &&
      !widget()->native_widget_private()->IsVisible())
    static_cast<NativeWindowViews*>(parent())->IncrementChildModals();

  widget()->native_widget_private()->Show(GetRestoredState(), gfx::Rect());

  // 显式聚焦窗口。
  widget()->Activate();

  NotifyWindowShow();

#if defined(USE_X11)
  if (global_menu_bar_)
    global_menu_bar_->OnWindowMapped();
#endif
}

void NativeWindowViews::ShowInactive() {
  widget()->ShowInactive();

  NotifyWindowShow();

#if defined(USE_X11)
  if (global_menu_bar_)
    global_menu_bar_->OnWindowMapped();
#endif
}

void NativeWindowViews::Hide() {
  if (is_modal() && NativeWindow::parent())
    static_cast<NativeWindowViews*>(parent())->DecrementChildModals();

  widget()->Hide();

  NotifyWindowHide();

#if defined(USE_X11)
  if (global_menu_bar_)
    global_menu_bar_->OnWindowUnmapped();
#endif

#if defined(OS_WIN)
  // 当通过win.ide()将窗口从任务栏移除时，
  // 需要重新设置缩略图按钮。
  // 确保在窗口隐藏时，
  // 系统会通知任务栏宿主它应该重新添加它们。
  taskbar_host_.SetThumbarButtonsAdded(false);
#endif
}

bool NativeWindowViews::IsVisible() {
  return widget()->IsVisible();
}

bool NativeWindowViews::IsEnabled() {
#if defined(OS_WIN)
  return ::IsWindowEnabled(GetAcceleratedWidget());
#elif defined(OS_LINUX)
#if defined(USE_X11)
  return !event_disabler_.get();
#endif
  NOTIMPLEMENTED();
  return true;
#endif
}

void NativeWindowViews::IncrementChildModals() {
  num_modal_children_++;
  SetEnabledInternal(ShouldBeEnabled());
}

void NativeWindowViews::DecrementChildModals() {
  if (num_modal_children_ > 0) {
    num_modal_children_--;
  }
  SetEnabledInternal(ShouldBeEnabled());
}

void NativeWindowViews::SetEnabled(bool enable) {
  if (enable != is_enabled_) {
    is_enabled_ = enable;
    SetEnabledInternal(ShouldBeEnabled());
  }
}

bool NativeWindowViews::ShouldBeEnabled() {
  return is_enabled_ && (num_modal_children_ == 0);
}

void NativeWindowViews::SetEnabledInternal(bool enable) {
  if (enable && IsEnabled()) {
    return;
  } else if (!enable && !IsEnabled()) {
    return;
  }

#if defined(OS_WIN)
  ::EnableWindow(GetAcceleratedWidget(), enable);
#elif defined(USE_X11)
  views::DesktopWindowTreeHostPlatform* tree_host =
      views::DesktopWindowTreeHostLinux::GetHostForWidget(
          GetAcceleratedWidget());
  if (enable) {
    tree_host->RemoveEventRewriter(event_disabler_.get());
    event_disabler_.reset();
  } else {
    event_disabler_ = std::make_unique<EventDisabler>();
    tree_host->AddEventRewriter(event_disabler_.get());
  }
#endif
}

#if defined(OS_LINUX)
void NativeWindowViews::Maximize() {
  if (IsVisible())
    widget()->Maximize();
  else
    widget()->native_widget_private()->Show(ui::SHOW_STATE_MAXIMIZED,
                                            gfx::Rect());
}
#endif

void NativeWindowViews::Unmaximize() {
#if defined(OS_WIN)
  if (transparent()) {
    SetBounds(restore_bounds_, false);
    return;
  }
#endif

  widget()->Restore();
}

bool NativeWindowViews::IsMaximized() {
  if (widget()->IsMaximized()) {
    return true;
  } else {
#if defined(OS_WIN)
    if (transparent()) {
      // 将窗口的大小与显示器的大小进行比较。
      auto display = display::Screen::GetScreen()->GetDisplayNearestWindow(
          GetNativeWindow());
      // 如果窗口的尺寸和位置与。
      // 显示。
      return GetBounds() == display.work_area();
    }
#endif

    return false;
  }
}

void NativeWindowViews::Minimize() {
  if (IsVisible())
    widget()->Minimize();
  else
    widget()->native_widget_private()->Show(ui::SHOW_STATE_MINIMIZED,
                                            gfx::Rect());
}

void NativeWindowViews::Restore() {
  widget()->Restore();
}

bool NativeWindowViews::IsMinimized() {
  return widget()->IsMinimized();
}

void NativeWindowViews::SetFullScreen(bool fullscreen) {
  if (!IsFullScreenable())
    return;

#if defined(OS_WIN)
  // Windows上没有本机全屏状态。
  bool leaving_fullscreen = IsFullscreen() && !fullscreen;

  if (fullscreen) {
    last_window_state_ = ui::SHOW_STATE_FULLSCREEN;
    NotifyWindowEnterFullScreen();
  } else {
    last_window_state_ = ui::SHOW_STATE_NORMAL;
    NotifyWindowLeaveFullScreen();
  }

  // 对于没有WS_THICKFRAME样式的窗口，我们不能调用SetFullScreen()。
  // 此路径也将用于透明窗口。
  if (!thick_frame_) {
    if (fullscreen) {
      restore_bounds_ = GetBounds();
      auto display =
          display::Screen::GetScreen()->GetDisplayNearestPoint(GetPosition());
      SetBounds(display.bounds(), false);
    } else {
      SetBounds(restore_bounds_, false);
    }
    return;
  }

  // 我们在通知之后设置新值，这样我们就可以处理Size事件。
  // 正确。
  widget()->SetFullscreen(fullscreen);

  // 如果从全屏恢复并且窗口不可见，则强制可见，
  // 否则，可以呈现无响应的窗口外壳。
  // (当APP以全屏方式启动时可能会出现这种情况：TRUE)。
  // 注意：以下内容必须在“widget()-&gt;SetFullScreen(Full Screen)；”之后；
  if (leaving_fullscreen && !IsVisible())
    FlipWindowStyle(GetAcceleratedWidget(), true, WS_VISIBLE);
#else
  if (IsVisible())
    widget()->SetFullscreen(fullscreen);
  else if (fullscreen)
    widget()->native_widget_private()->Show(ui::SHOW_STATE_FULLSCREEN,
                                            gfx::Rect());

  // 全屏时自动隐藏菜单栏。
  if (fullscreen)
    SetMenuBarVisibility(false);
  else
    SetMenuBarVisibility(!IsMenuBarAutoHide());
#endif
}

bool NativeWindowViews::IsFullscreen() const {
  return widget()->IsFullscreen();
}

void NativeWindowViews::SetBounds(const gfx::Rect& bounds, bool animate) {
#if defined(OS_WIN) || defined(OS_LINUX)
  // 在Linux和Windows上，最小和最大大小应更新为。
  // 当窗口不可调整大小时的窗口大小。
  if (!resizable_) {
    SetMaximumSize(bounds.size());
    SetMinimumSize(bounds.size());
  }
#endif

  widget()->SetBounds(bounds);
}

gfx::Rect NativeWindowViews::GetBounds() {
#if defined(OS_WIN)
  if (IsMinimized())
    return widget()->GetRestoredBounds();
#endif

  return widget()->GetWindowBoundsInScreen();
}

gfx::Rect NativeWindowViews::GetContentBounds() {
  return content_view() ? content_view()->GetBoundsInScreen() : gfx::Rect();
}

gfx::Size NativeWindowViews::GetContentSize() {
#if defined(OS_WIN)
  if (IsMinimized())
    return NativeWindow::GetContentSize();
#endif

  return content_view() ? content_view()->size() : gfx::Size();
}

gfx::Rect NativeWindowViews::GetNormalBounds() {
  return widget()->GetRestoredBounds();
}

void NativeWindowViews::SetContentSizeConstraints(
    const extensions::SizeConstraints& size_constraints) {
  NativeWindow::SetContentSizeConstraints(size_constraints);
#if defined(OS_WIN)
  // 更改大小约束将强制添加WS_THICKFRAME样式，因此。
  // 如果ThickFrame为false，则不执行任何操作。
  if (!thick_frame_)
    return;
#endif
  // Widget_Delegate()只有在调用Init()之后才可用，我们利用。
  // 这将确定本机小部件是否已初始化。
  if (widget() && widget()->widget_delegate())
    widget()->OnSizeConstraintsChanged();
  if (resizable_)
    old_size_constraints_ = size_constraints;
}

void NativeWindowViews::SetResizable(bool resizable) {
  if (resizable != resizable_) {
    // 在Linux上，窗口没有“可调整大小”属性，我们必须设置。
    // 将最小尺寸和最大尺寸都调到窗口大小才能实现。
    if (resizable) {
      SetContentSizeConstraints(old_size_constraints_);
      SetMaximizable(maximizable_);
    } else {
      old_size_constraints_ = GetContentSizeConstraints();
      resizable_ = false;
      gfx::Size content_size = GetContentSize();
      SetContentSizeConstraints(
          extensions::SizeConstraints(content_size, content_size));
    }
  }
#if defined(OS_WIN)
  if (has_frame() && thick_frame_)
    FlipWindowStyle(GetAcceleratedWidget(), resizable, WS_THICKFRAME);
#endif
  resizable_ = resizable;
  SetCanResize(resizable_);
}

bool NativeWindowViews::MoveAbove(const std::string& sourceId) {
  const content::DesktopMediaID id = content::DesktopMediaID::Parse(sourceId);
  if (id.type != content::DesktopMediaID::TYPE_WINDOW)
    return false;

#if defined(OS_WIN)
  const HWND otherWindow = reinterpret_cast<HWND>(id.id);
  if (!::IsWindow(otherWindow))
    return false;

  ::SetWindowPos(GetAcceleratedWidget(), GetWindow(otherWindow, GW_HWNDPREV), 0,
                 0, 0, 0,
                 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#elif defined(USE_X11)
  if (!IsWindowValid(static_cast<x11::Window>(id.id)))
    return false;

  electron::MoveWindowAbove(static_cast<x11::Window>(GetAcceleratedWidget()),
                            static_cast<x11::Window>(id.id));
#endif

  return true;
}

void NativeWindowViews::MoveTop() {
// TODO(julien.isorce)：修复铬以便使用现有的。
// Widget()-&gt;StackAtTop()。
#if defined(OS_WIN)
  gfx::Point pos = GetPosition();
  gfx::Size size = GetSize();
  ::SetWindowPos(GetAcceleratedWidget(), HWND_TOP, pos.x(), pos.y(),
                 size.width(), size.height(),
                 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#elif defined(USE_X11)
  electron::MoveWindowToForeground(
      static_cast<x11::Window>(GetAcceleratedWidget()));
#endif
}

bool NativeWindowViews::IsResizable() {
#if defined(OS_WIN)
  if (has_frame())
    return ::GetWindowLong(GetAcceleratedWidget(), GWL_STYLE) & WS_THICKFRAME;
#endif
  return resizable_;
}

void NativeWindowViews::SetAspectRatio(double aspect_ratio,
                                       const gfx::Size& extra_size) {
  NativeWindow::SetAspectRatio(aspect_ratio, extra_size);
  gfx::SizeF aspect(aspect_ratio, 1.0);
  // 向上扩展，因为SetAspectRatio()将纵横比值截断为整型。
  aspect.Scale(100);

  widget()->SetAspectRatio(aspect);
}

void NativeWindowViews::SetMovable(bool movable) {
  movable_ = movable;
}

bool NativeWindowViews::IsMovable() {
#if defined(OS_WIN)
  return movable_;
#else
  return true;  // 未在Linux上实现。
#endif
}

void NativeWindowViews::SetMinimizable(bool minimizable) {
#if defined(OS_WIN)
  FlipWindowStyle(GetAcceleratedWidget(), minimizable, WS_MINIMIZEBOX);
#endif
  minimizable_ = minimizable;
}

bool NativeWindowViews::IsMinimizable() {
#if defined(OS_WIN)
  return ::GetWindowLong(GetAcceleratedWidget(), GWL_STYLE) & WS_MINIMIZEBOX;
#else
  return true;  // 未在Linux上实现。
#endif
}

void NativeWindowViews::SetMaximizable(bool maximizable) {
#if defined(OS_WIN)
  FlipWindowStyle(GetAcceleratedWidget(), maximizable, WS_MAXIMIZEBOX);
#endif
  maximizable_ = maximizable;
}

bool NativeWindowViews::IsMaximizable() {
#if defined(OS_WIN)
  return ::GetWindowLong(GetAcceleratedWidget(), GWL_STYLE) & WS_MAXIMIZEBOX;
#else
  return true;  // 未在Linux上实现。
#endif
}

void NativeWindowViews::SetExcludedFromShownWindowsMenu(bool excluded) {}

bool NativeWindowViews::IsExcludedFromShownWindowsMenu() {
  // 在不支持的平台上返回False。
  return false;
}

void NativeWindowViews::SetFullScreenable(bool fullscreenable) {
  fullscreenable_ = fullscreenable;
}

bool NativeWindowViews::IsFullScreenable() {
  return fullscreenable_;
}

void NativeWindowViews::SetClosable(bool closable) {
#if defined(OS_WIN)
  HMENU menu = GetSystemMenu(GetAcceleratedWidget(), false);
  if (closable) {
    EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
  } else {
    EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
  }
#endif
}

bool NativeWindowViews::IsClosable() {
#if defined(OS_WIN)
  HMENU menu = GetSystemMenu(GetAcceleratedWidget(), false);
  MENUITEMINFO info;
  memset(&info, 0, sizeof(info));
  info.cbSize = sizeof(info);
  info.fMask = MIIM_STATE;
  if (!GetMenuItemInfo(menu, SC_CLOSE, false, &info)) {
    return false;
  }
  return !(info.fState & MFS_DISABLED);
#elif defined(OS_LINUX)
  return true;
#endif
}

void NativeWindowViews::SetAlwaysOnTop(ui::ZOrderLevel z_order,
                                       const std::string& level,
                                       int relativeLevel) {
  bool level_changed = z_order != widget()->GetZOrderLevel();
  widget()->SetZOrderLevel(z_order);

#if defined(OS_WIN)
  // 重置放置标志。
  behind_task_bar_ = false;
  if (z_order != ui::ZOrderLevel::kNormal) {
    // 在MacOS上，该窗口位于Dock后面的以下级别。
    // 在Windows上重复使用相同的名称以便于用户使用。
    static const std::vector<std::string> levels = {
        "floating", "torn-off-menu", "modal-panel", "main-menu", "status"};
    behind_task_bar_ = base::Contains(levels, level);
  }
#endif
  MoveBehindTaskBarIfNeeded();

  // 这必须在最后或IsAlwaysOnTop通知。
  // 将尚未更新以反映新状态。
  if (level_changed)
    NativeWindow::NotifyWindowAlwaysOnTopChanged();
}

ui::ZOrderLevel NativeWindowViews::GetZOrderLevel() {
  return widget()->GetZOrderLevel();
}

void NativeWindowViews::Center() {
  widget()->CenterWindow(GetSize());
}

void NativeWindowViews::Invalidate() {
  widget()->SchedulePaintInRect(gfx::Rect(GetBounds().size()));
}

void NativeWindowViews::SetTitle(const std::string& title) {
  title_ = title;
  widget()->UpdateWindowTitle();
}

std::string NativeWindowViews::GetTitle() {
  return title_;
}

void NativeWindowViews::FlashFrame(bool flash) {
#if defined(OS_WIN)
  // Chromium的实现有一个停止闪烁的错误。
  if (!flash) {
    FLASHWINFO fwi;
    fwi.cbSize = sizeof(fwi);
    fwi.hwnd = GetAcceleratedWidget();
    fwi.dwFlags = FLASHW_STOP;
    fwi.uCount = 0;
    FlashWindowEx(&fwi);
    return;
  }
#endif
  widget()->FlashFrame(flash);
}

void NativeWindowViews::SetSkipTaskbar(bool skip) {
#if defined(OS_WIN)
  Microsoft::WRL::ComPtr<ITaskbarList> taskbar;
  if (FAILED(::CoCreateInstance(CLSID_TaskbarList, nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&taskbar))) ||
      FAILED(taskbar->HrInit()))
    return;
  if (skip) {
    taskbar->DeleteTab(GetAcceleratedWidget());
  } else {
    taskbar->AddTab(GetAcceleratedWidget());
    taskbar_host_.RestoreThumbarButtons(GetAcceleratedWidget());
  }
#elif defined(USE_X11)
  SetWMSpecState(static_cast<x11::Window>(GetAcceleratedWidget()), skip,
                 x11::GetAtom("_NET_WM_STATE_SKIP_TASKBAR"));
#endif
}

void NativeWindowViews::SetSimpleFullScreen(bool simple_fullscreen) {
  SetFullScreen(simple_fullscreen);
}

bool NativeWindowViews::IsSimpleFullScreen() {
  return IsFullscreen();
}

void NativeWindowViews::SetKiosk(bool kiosk) {
  SetFullScreen(kiosk);
}

bool NativeWindowViews::IsKiosk() {
  return IsFullscreen();
}

bool NativeWindowViews::IsTabletMode() const {
#if defined(OS_WIN)
  return base::win::IsWindows10TabletMode(GetAcceleratedWidget());
#else
  return false;
#endif
}

SkColor NativeWindowViews::GetBackgroundColor() {
  auto* background = root_view_->background();
  if (!background)
    return SK_ColorTRANSPARENT;
  return background->get_color();
}

void NativeWindowViews::SetBackgroundColor(SkColor background_color) {
  // Web视图的背景色。
  root_view_->SetBackground(views::CreateSolidBackground(background_color));

#if defined(OS_WIN)
  // 设置本机窗口的背景色。
  HBRUSH brush = CreateSolidBrush(skia::SkColorToCOLORREF(background_color));
  ULONG_PTR previous_brush =
      SetClassLongPtr(GetAcceleratedWidget(), GCLP_HBRBACKGROUND,
                      reinterpret_cast<LONG_PTR>(brush));
  if (previous_brush)
    DeleteObject((HBRUSH)previous_brush);
  InvalidateRect(GetAcceleratedWidget(), NULL, 1);
#endif
}

void NativeWindowViews::SetHasShadow(bool has_shadow) {
  wm::SetShadowElevation(GetNativeWindow(),
                         has_shadow ? wm::kShadowElevationInactiveWindow
                                    : wm::kShadowElevationNone);
}

bool NativeWindowViews::HasShadow() {
  return GetNativeWindow()->GetProperty(wm::kShadowElevationKey) !=
         wm::kShadowElevationNone;
}

void NativeWindowViews::SetOpacity(const double opacity) {
#if defined(OS_WIN)
  const double boundedOpacity = base::clamp(opacity, 0.0, 1.0);
  HWND hwnd = GetAcceleratedWidget();
  if (!layered_) {
    LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
    ex_style |= WS_EX_LAYERED;
    ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    layered_ = true;
  }
  ::SetLayeredWindowAttributes(hwnd, 0, boundedOpacity * 255, LWA_ALPHA);
  opacity_ = boundedOpacity;
#else
  opacity_ = 1.0;  // Linux上不支持setOpacity。
#endif
}

double NativeWindowViews::GetOpacity() {
  return opacity_;
}

void NativeWindowViews::SetIgnoreMouseEvents(bool ignore, bool forward) {
#if defined(OS_WIN)
  LONG ex_style = ::GetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE);
  if (ignore)
    ex_style |= (WS_EX_TRANSPARENT | WS_EX_LAYERED);
  else
    ex_style &= ~(WS_EX_TRANSPARENT | WS_EX_LAYERED);
  if (layered_)
    ex_style |= WS_EX_LAYERED;
  ::SetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE, ex_style);

  // 当未忽略鼠标消息时，转发始终处于禁用状态。
  if (!ignore) {
    SetForwardMouseMessages(false);
  } else {
    SetForwardMouseMessages(forward);
  }
#elif defined(USE_X11)
  auto* connection = x11::Connection::Get();
  if (ignore) {
    x11::Rectangle r{0, 0, 1, 1};
    connection->shape().Rectangles({
        .operation = x11::Shape::So::Set,
        .destination_kind = x11::Shape::Sk::Input,
        .ordering = x11::ClipOrdering::YXBanded,
        .destination_window = static_cast<x11::Window>(GetAcceleratedWidget()),
        .rectangles = {r},
    });
  } else {
    connection->shape().Mask({
        .operation = x11::Shape::So::Set,
        .destination_kind = x11::Shape::Sk::Input,
        .destination_window = static_cast<x11::Window>(GetAcceleratedWidget()),
        .source_bitmap = x11::Pixmap::None,
    });
  }
#endif
}

void NativeWindowViews::SetContentProtection(bool enable) {
#if defined(OS_WIN)
  HWND hwnd = GetAcceleratedWidget();
  if (!layered_) {
    // 解决方法，以防止在隐藏和显示屏幕截图后出现黑色窗口。
    // 显示浏览器窗口。
    LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
    ex_style |= WS_EX_LAYERED;
    ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    layered_ = true;
  }
  DWORD affinity = enable ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
  ::SetWindowDisplayAffinity(hwnd, affinity);
#endif
}

void NativeWindowViews::SetFocusable(bool focusable) {
  widget()->widget_delegate()->SetCanActivate(focusable);
#if defined(OS_WIN)
  LONG ex_style = ::GetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE);
  if (focusable)
    ex_style &= ~WS_EX_NOACTIVATE;
  else
    ex_style |= WS_EX_NOACTIVATE;
  ::SetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE, ex_style);
  SetSkipTaskbar(!focusable);
  Focus(false);
#endif
}

bool NativeWindowViews::IsFocusable() {
  bool can_activate = widget()->widget_delegate()->CanActivate();
#if defined(OS_WIN)
  LONG ex_style = ::GetWindowLong(GetAcceleratedWidget(), GWL_EXSTYLE);
  bool no_activate = ex_style & WS_EX_NOACTIVATE;
  return !no_activate && can_activate;
#else
  return can_activate;
#endif
}

void NativeWindowViews::SetMenu(ElectronMenuModel* menu_model) {
#if defined(USE_X11)
  // 删除全局菜单栏。
  if (global_menu_bar_ && menu_model == nullptr) {
    global_menu_bar_.reset();
    root_view_->UnregisterAcceleratorsWithFocusManager();
    return;
  }

  // 尽可能使用全局应用程序菜单栏。
  if (ShouldUseGlobalMenuBar()) {
    if (!global_menu_bar_)
      global_menu_bar_ = std::make_unique<GlobalMenuBarX11>(this);
    if (global_menu_bar_->IsServerStarted()) {
      root_view_->RegisterAcceleratorsWithFocusManager(menu_model);
      global_menu_bar_->SetMenu(menu_model);
      return;
    }
  }
#endif

  // 设置菜单时应重置内容大小。
  gfx::Size content_size = GetContentSize();
  bool should_reset_size = use_content_size_ && has_frame() &&
                           !IsMenuBarAutoHide() &&
                           ((!!menu_model) != root_view_->HasMenu());

  root_view_->SetMenu(menu_model);

  if (should_reset_size) {
    // 放大菜单的大小约束。
    int menu_bar_height = root_view_->GetMenuBarHeight();
    extensions::SizeConstraints constraints = GetContentSizeConstraints();
    if (constraints.HasMinimumSize()) {
      gfx::Size min_size = constraints.GetMinimumSize();
      min_size.set_height(min_size.height() + menu_bar_height);
      constraints.set_minimum_size(min_size);
    }
    if (constraints.HasMaximumSize()) {
      gfx::Size max_size = constraints.GetMaximumSize();
      max_size.set_height(max_size.height() + menu_bar_height);
      constraints.set_maximum_size(max_size);
    }
    SetContentSizeConstraints(constraints);

    // 调整窗口大小以确保内容大小不变。
    SetContentSize(content_size);
  }
}

void NativeWindowViews::AddBrowserView(NativeBrowserView* view) {
  if (!content_view())
    return;

  if (!view) {
    return;
  }

  add_browser_view(view);
  if (view->GetInspectableWebContentsView())
    content_view()->AddChildView(
        view->GetInspectableWebContentsView()->GetView());
}

void NativeWindowViews::RemoveBrowserView(NativeBrowserView* view) {
  if (!content_view())
    return;

  if (!view) {
    return;
  }

  if (view->GetInspectableWebContentsView())
    content_view()->RemoveChildView(
        view->GetInspectableWebContentsView()->GetView());
  remove_browser_view(view);
}

void NativeWindowViews::SetTopBrowserView(NativeBrowserView* view) {
  if (!content_view())
    return;

  if (!view) {
    return;
  }

  remove_browser_view(view);
  add_browser_view(view);

  if (view->GetInspectableWebContentsView())
    content_view()->ReorderChildView(
        view->GetInspectableWebContentsView()->GetView(), -1);
}

void NativeWindowViews::SetParentWindow(NativeWindow* parent) {
  NativeWindow::SetParentWindow(parent);

#if defined(USE_X11)
  x11::SetProperty(
      static_cast<x11::Window>(GetAcceleratedWidget()),
      x11::Atom::WM_TRANSIENT_FOR, x11::Atom::WINDOW,
      parent ? static_cast<x11::Window>(parent->GetAcceleratedWidget())
             : ui::GetX11RootWindow());
#elif defined(OS_WIN)
  // 要将窗口之间的父子关系设置为Windows，最好使用。
  // 所有者，而不是父级，如果父级。
  // 在窗口创建时指定。
  // 为此，我们不能使用：：SetParent函数，而必须使用。
  // 设置了“nIndex”的：：GetWindowLongPtr或：：SetWindowLongPtr函数。
  // 设置为“GWLP_HWNDPARENT”，它实际上表示窗口所有者。
  HWND hwndParent = parent ? parent->GetAcceleratedWidget() : NULL;
  if (hwndParent ==
      (HWND)::GetWindowLongPtr(GetAcceleratedWidget(), GWLP_HWNDPARENT))
    return;
  ::SetWindowLongPtr(GetAcceleratedWidget(), GWLP_HWNDPARENT,
                     (LONG_PTR)hwndParent);
  // 确保可见性。
  if (IsVisible()) {
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    ::GetWindowPlacement(GetAcceleratedWidget(), &wp);
    ::ShowWindow(GetAcceleratedWidget(), SW_HIDE);
    ::ShowWindow(GetAcceleratedWidget(), wp.showCmd);
    ::BringWindowToTop(GetAcceleratedWidget());
  }
#endif
}

gfx::NativeView NativeWindowViews::GetNativeView() const {
  return widget()->GetNativeView();
}

gfx::NativeWindow NativeWindowViews::GetNativeWindow() const {
  return widget()->GetNativeWindow();
}

void NativeWindowViews::SetProgressBar(double progress,
                                       NativeWindow::ProgressState state) {
#if defined(OS_WIN)
  taskbar_host_.SetProgressBar(GetAcceleratedWidget(), progress, state);
#elif defined(OS_LINUX)
  if (unity::IsRunning()) {
    unity::SetProgressFraction(progress);
  }
#endif
}

void NativeWindowViews::SetOverlayIcon(const gfx::Image& overlay,
                                       const std::string& description) {
#if defined(OS_WIN)
  SkBitmap overlay_bitmap = overlay.AsBitmap();
  taskbar_host_.SetOverlayIcon(GetAcceleratedWidget(), overlay_bitmap,
                               description);
#endif
}

void NativeWindowViews::SetAutoHideMenuBar(bool auto_hide) {
  root_view_->SetAutoHideMenuBar(auto_hide);
}

bool NativeWindowViews::IsMenuBarAutoHide() {
  return root_view_->IsMenuBarAutoHide();
}

void NativeWindowViews::SetMenuBarVisibility(bool visible) {
  root_view_->SetMenuBarVisibility(visible);
}

bool NativeWindowViews::IsMenuBarVisible() {
  return root_view_->IsMenuBarVisible();
}

void NativeWindowViews::SetVisibleOnAllWorkspaces(
    bool visible,
    bool visibleOnFullScreen,
    bool skipTransformProcessType) {
  widget()->SetVisibleOnAllWorkspaces(visible);
}

bool NativeWindowViews::IsVisibleOnAllWorkspaces() {
#if defined(USE_X11)
  // 使用_NET_WM_STATE_STATE_STATE中的_NET_WM_STATE_SICKY的存在/缺失来执行以下操作：
  // 确定当前窗口是否在所有工作区中可见。
  x11::Atom sticky_atom = x11::GetAtom("_NET_WM_STATE_STICKY");
  std::vector<x11::Atom> wm_states;
  GetArrayProperty(static_cast<x11::Window>(GetAcceleratedWidget()),
                   x11::GetAtom("_NET_WM_STATE"), &wm_states);
  return std::find(wm_states.begin(), wm_states.end(), sticky_atom) !=
         wm_states.end();
#else
  return false;
#endif
}

content::DesktopMediaID NativeWindowViews::GetDesktopMediaID() const {
  const gfx::AcceleratedWidget accelerated_widget = GetAcceleratedWidget();
  content::DesktopMediaID::Id window_handle = content::DesktopMediaID::kNullId;
  content::DesktopMediaID::Id aura_id = content::DesktopMediaID::kNullId;
#if defined(OS_WIN)
  window_handle =
      reinterpret_cast<content::DesktopMediaID::Id>(accelerated_widget);
#elif defined(OS_LINUX)
  window_handle = static_cast<uint32_t>(accelerated_widget);
#endif
  aura::WindowTreeHost* const host =
      aura::WindowTreeHost::GetForAcceleratedWidget(accelerated_widget);
  aura::Window* const aura_window = host ? host->window() : nullptr;
  if (aura_window) {
    aura_id = content::DesktopMediaID::RegisterNativeWindow(
                  content::DesktopMediaID::TYPE_WINDOW, aura_window)
                  .window_id;
  }

  // 没有要传递aura_id的构造函数。一定不要用另一个。
  // 有第三个参数的构造函数，它还有另一个用途。
  content::DesktopMediaID result = content::DesktopMediaID(
      content::DesktopMediaID::TYPE_WINDOW, window_handle);

  // 令人困惑，但这就是Content：：DesktopMediaID的设计方式。本ID。
  // 属性是窗口句柄，而window_id属性是id。
  // 由包含所有气场实例的贴图给出。
  result.window_id = aura_id;
  return result;
}

gfx::AcceleratedWidget NativeWindowViews::GetAcceleratedWidget() const {
  if (GetNativeWindow() && GetNativeWindow()->GetHost())
    return GetNativeWindow()->GetHost()->GetAcceleratedWidget();
  else
    return gfx::kNullAcceleratedWidget;
}

NativeWindowHandle NativeWindowViews::GetNativeWindowHandle() const {
  return GetAcceleratedWidget();
}

gfx::Rect NativeWindowViews::ContentBoundsToWindowBounds(
    const gfx::Rect& bounds) const {
  if (!has_frame())
    return bounds;

  gfx::Rect window_bounds(bounds);
#if defined(OS_WIN)
  if (widget()->non_client_view()) {
    HWND hwnd = GetAcceleratedWidget();
    gfx::Rect dpi_bounds = DIPToScreenRect(hwnd, bounds);
    window_bounds = ScreenToDIPRect(
        hwnd, widget()->non_client_view()->GetWindowBoundsForClientBounds(
                  dpi_bounds));
  }
#endif

  if (root_view_->HasMenu() && root_view_->IsMenuBarVisible()) {
    int menu_bar_height = root_view_->GetMenuBarHeight();
    window_bounds.set_y(window_bounds.y() - menu_bar_height);
    window_bounds.set_height(window_bounds.height() + menu_bar_height);
  }
  return window_bounds;
}

gfx::Rect NativeWindowViews::WindowBoundsToContentBounds(
    const gfx::Rect& bounds) const {
  if (!has_frame())
    return bounds;

  gfx::Rect content_bounds(bounds);
#if defined(OS_WIN)
  HWND hwnd = GetAcceleratedWidget();
  content_bounds.set_size(DIPToScreenRect(hwnd, content_bounds).size());
  RECT rect;
  SetRectEmpty(&rect);
  DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
  DWORD ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
  AdjustWindowRectEx(&rect, style, FALSE, ex_style);
  content_bounds.set_width(content_bounds.width() - (rect.right - rect.left));
  content_bounds.set_height(content_bounds.height() - (rect.bottom - rect.top));
  content_bounds.set_size(ScreenToDIPRect(hwnd, content_bounds).size());
#endif

  if (root_view_->HasMenu() && root_view_->IsMenuBarVisible()) {
    int menu_bar_height = root_view_->GetMenuBarHeight();
    content_bounds.set_y(content_bounds.y() + menu_bar_height);
    content_bounds.set_height(content_bounds.height() - menu_bar_height);
  }
  return content_bounds;
}

void NativeWindowViews::UpdateDraggableRegions(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  draggable_region_ = DraggableRegionsToSkRegion(regions);
}

#if defined(OS_WIN)
void NativeWindowViews::SetIcon(HICON window_icon, HICON app_icon) {
  // 我们负责存储图像。
  window_icon_ = base::win::ScopedHICON(CopyIcon(window_icon));
  app_icon_ = base::win::ScopedHICON(CopyIcon(app_icon));

  HWND hwnd = GetAcceleratedWidget();
  SendMessage(hwnd, WM_SETICON, ICON_SMALL,
              reinterpret_cast<LPARAM>(window_icon_.get()));
  SendMessage(hwnd, WM_SETICON, ICON_BIG,
              reinterpret_cast<LPARAM>(app_icon_.get()));
}
#elif defined(OS_LINUX)
void NativeWindowViews::SetIcon(const gfx::ImageSkia& icon) {
  auto* tree_host = views::DesktopWindowTreeHostLinux::GetHostForWidget(
      GetAcceleratedWidget());
  tree_host->SetWindowIcons(icon, {});
}
#endif

void NativeWindowViews::OnWidgetActivationChanged(views::Widget* changed_widget,
                                                  bool active) {
  if (changed_widget != widget())
    return;

  if (active) {
    MoveBehindTaskBarIfNeeded();
    NativeWindow::NotifyWindowFocus();
  } else {
    NativeWindow::NotifyWindowBlur();
  }

  // 当窗口模糊时隐藏菜单栏。
  if (!active && IsMenuBarAutoHide() && IsMenuBarVisible())
    SetMenuBarVisibility(false);

  root_view_->ResetAltState();
}

void NativeWindowViews::OnWidgetBoundsChanged(views::Widget* changed_widget,
                                              const gfx::Rect& bounds) {
  if (changed_widget != widget())
    return;

  // 注意：我们故意使用`GetBound()`而不是`bons`来正确地。
  // 处理Windows上的最小化窗口。
  const auto new_bounds = GetBounds();
  if (widget_size_ != new_bounds.size()) {
    int width_delta = new_bounds.width() - widget_size_.width();
    int height_delta = new_bounds.height() - widget_size_.height();
    for (NativeBrowserView* item : browser_views()) {
      auto* native_view = static_cast<NativeBrowserViewViews*>(item);
      native_view->SetAutoResizeProportions(widget_size_);
      native_view->AutoResize(new_bounds, width_delta, height_delta);
    }

    NotifyWindowResize();
    widget_size_ = new_bounds.size();
  }
}

void NativeWindowViews::OnWidgetDestroying(views::Widget* widget) {
  aura::Window* window = GetNativeWindow();
  if (window)
    window->RemovePreTargetHandler(this);
}

void NativeWindowViews::OnWidgetDestroyed(views::Widget* changed_widget) {
  widget_destroyed_ = true;
}

views::View* NativeWindowViews::GetInitiallyFocusedView() {
  return focused_view_;
}

bool NativeWindowViews::CanMaximize() const {
  return resizable_ && maximizable_;
}

bool NativeWindowViews::CanMinimize() const {
#if defined(OS_WIN)
  return minimizable_;
#elif defined(OS_LINUX)
  return true;
#endif
}

std::u16string NativeWindowViews::GetWindowTitle() const {
  return base::UTF8ToUTF16(title_);
}

views::View* NativeWindowViews::GetContentsView() {
  return root_view_.get();
}

bool NativeWindowViews::ShouldDescendIntoChildForEventHandling(
    gfx::NativeView child,
    const gfx::Point& location) {
  // 应用程序窗口应声明属于任何BrowserViews的鼠标事件。
  // 可拖动区域。
  for (auto* view : browser_views()) {
    auto* native_view = static_cast<NativeBrowserViewViews*>(view);
    auto* view_draggable_region = native_view->draggable_region();
    if (view_draggable_region &&
        view_draggable_region->contains(location.x(), location.y()))
      return false;
  }

  // 应用程序窗口应声明落在可拖动区域内的鼠标事件。
  if (draggable_region() &&
      draggable_region()->contains(location.x(), location.y()))
    return false;

  // 以及用于拖动可调整大小的无边框窗口的边框上事件。
  if (!has_frame() && resizable_) {
    auto* frame =
        static_cast<FramelessView*>(widget()->non_client_view()->frame_view());
    return frame->ResizingBorderHitTest(location) == HTNOWHERE;
  }

  return true;
}

views::ClientView* NativeWindowViews::CreateClientView(views::Widget* widget) {
  return new NativeWindowClientView(widget, root_view_.get(), this);
}

std::unique_ptr<views::NonClientFrameView>
NativeWindowViews::CreateNonClientFrameView(views::Widget* widget) {
#if defined(OS_WIN)
  auto frame_view = std::make_unique<WinFrameView>();
  frame_view->Init(this, widget);
  return frame_view;
#else
  if (has_frame()) {
    return std::make_unique<NativeFrameView>(this, widget);
  } else {
    auto frame_view = std::make_unique<FramelessView>();
    frame_view->Init(this, widget);
    return frame_view;
  }
#endif
}

void NativeWindowViews::OnWidgetMove() {
  NotifyWindowMove();
}

void NativeWindowViews::HandleKeyboardEvent(
    content::WebContents*,
    const content::NativeWebKeyboardEvent& event) {
  if (widget_destroyed_)
    return;

#if defined(OS_LINUX)
  if (event.windows_key_code == ui::VKEY_BROWSER_BACK)
    NotifyWindowExecuteAppCommand(kBrowserBackward);
  else if (event.windows_key_code == ui::VKEY_BROWSER_FORWARD)
    NotifyWindowExecuteAppCommand(kBrowserForward);
#endif

  keyboard_event_handler_->HandleKeyboardEvent(event,
                                               root_view_->GetFocusManager());
  root_view_->HandleKeyEvent(event);
}

void NativeWindowViews::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() != ui::ET_MOUSE_PRESSED)
    return;

  // 按住Alt键并单击不应切换菜单栏。
  root_view_->ResetAltState();

#if defined(OS_LINUX)
  if (event->changed_button_flags() == ui::EF_BACK_MOUSE_BUTTON)
    NotifyWindowExecuteAppCommand(kBrowserBackward);
  else if (event->changed_button_flags() == ui::EF_FORWARD_MOUSE_BUTTON)
    NotifyWindowExecuteAppCommand(kBrowserForward);
#endif
}

ui::WindowShowState NativeWindowViews::GetRestoredState() {
  if (IsMaximized()) {
#if defined(OS_WIN)
    // 仅当窗口不是透明样式时才恢复最大化状态。
    if (!transparent()) {
      return ui::SHOW_STATE_MAXIMIZED;
    }
#else
    return ui::SHOW_STATE_MAXIMIZED;
#endif
  }

  if (IsFullscreen())
    return ui::SHOW_STATE_FULLSCREEN;

  return ui::SHOW_STATE_NORMAL;
}

void NativeWindowViews::MoveBehindTaskBarIfNeeded() {
#if defined(OS_WIN)
  if (behind_task_bar_) {
    const HWND task_bar_hwnd = ::FindWindow(kUniqueTaskBarClassName, nullptr);
    ::SetWindowPos(GetAcceleratedWidget(), task_bar_hwnd, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  }
#endif
  // TODO(julien.isorce)：实现X11案例。
}

// 静电。
NativeWindow* NativeWindow::Create(const gin_helper::Dictionary& options,
                                   NativeWindow* parent) {
  return new NativeWindowViews(options, parent);
}

}  // 命名空间电子
