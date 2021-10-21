// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_browser_window.h"

#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"  // 点名检查。
#include "content/browser/renderer_host/render_widget_host_owner_delegate.h"  // 点名检查。
#include "content/browser/web_contents/web_contents_impl.h"  // 点名检查。
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "shell/browser/api/electron_api_web_contents_view.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_browser_view.h"
#include "shell/browser/unresponsive_suppressor.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/browser/window_list.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "ui/gl/gpu_switching_manager.h"

namespace electron {

namespace api {

BrowserWindow::BrowserWindow(gin::Arguments* args,
                             const gin_helper::Dictionary& options)
    : BaseWindow(args->isolate(), options) {
  // 在WebContents中使用options.webPreferences。
  v8::Isolate* isolate = args->isolate();
  gin_helper::Dictionary web_preferences =
      gin::Dictionary::CreateEmpty(isolate);
  options.Get(options::kWebPreferences, &web_preferences);

  // 将backround Color复制到webContents。
  v8::Local<v8::Value> value;
  bool transparent = false;
  if (options.Get(options::kBackgroundColor, &value)) {
    web_preferences.SetHidden(options::kBackgroundColor, value);
  } else if (options.Get(options::kTransparent, &transparent) && transparent) {
    // 如果BrowserWindow是透明的，还会将透明度传播到。
    // WebContents，除非设置了单独的backmentColor。
    web_preferences.SetHidden(options::kBackgroundColor,
                              ToRGBAHex(SK_ColorTRANSPARENT));
  }

  // 将show设置复制到webContents，但前提是我们不想绘制。
  // 最初隐藏时。
  bool paint_when_initially_hidden = true;
  options.Get("paintWhenInitiallyHidden", &paint_when_initially_hidden);
  if (!paint_when_initially_hidden) {
    bool show = true;
    options.Get(options::kShow, &show);
    web_preferences.Set(options::kShow, show);
  }

  bool titleBarOverlay = false;
  options.Get(options::ktitleBarOverlay, &titleBarOverlay);
  if (titleBarOverlay) {
    std::string enabled_features = "";
    if (web_preferences.Get(options::kEnableBlinkFeatures, &enabled_features)) {
      enabled_features += ",";
    }
    enabled_features += features::kWebAppWindowControlsOverlay.name;
    web_preferences.Set(options::kEnableBlinkFeatures, enabled_features);
  }

  // 将webContents选项复制到webPreferences。这仅供内部使用。
  // 若要实现nativeWindowOpen选项，请执行以下操作。
  if (options.Get("webContents", &value)) {
    web_preferences.SetHidden("webContents", value);
  }

  // 创建WebContentsView。
  gin::Handle<WebContentsView> web_contents_view =
      WebContentsView::Create(isolate, web_preferences);
  DCHECK(web_contents_view.get());

  // 保存WebContents的引用。
  gin::Handle<WebContents> web_contents =
      web_contents_view->GetWebContents(isolate);
  web_contents_.Reset(isolate, web_contents.ToV8());
  api_web_contents_ = web_contents->GetWeakPtr();
  api_web_contents_->AddObserver(this);
  Observe(api_web_contents_->web_contents());

  // 与BrowserWindow关联。
  web_contents->SetOwnerWindow(window());

  auto* host = web_contents->web_contents()->GetRenderViewHost();
  if (host)
    host->GetWidget()->AddInputEventObserver(this);

  InitWithArgs(args);

  // 在初始化BaseWindow的JS代码后安装内容视图。
  SetContentView(gin::CreateHandle<View>(isolate, web_contents_view.get()));

#if defined(OS_MAC)
  OverrideNSWindowContentView(
      web_contents->inspectable_web_contents()->GetView());
#endif

  // 一切设置完成后初始化窗口。
  window()->InitFromOptions(options);
}

BrowserWindow::~BrowserWindow() {
  if (api_web_contents_) {
    // 如果用户直接销毁此实例，而不是。
    // 优雅地关闭Content：：WebContents。
    auto* host = web_contents()->GetRenderViewHost();
    if (host)
      host->GetWidget()->RemoveInputEventObserver(this);
    api_web_contents_->RemoveObserver(this);
    // 销毁Web内容。
    OnCloseContents();
  }
}

void BrowserWindow::OnInputEvent(const blink::WebInputEvent& event) {
  switch (event.GetType()) {
    case blink::WebInputEvent::Type::kGestureScrollBegin:
    case blink::WebInputEvent::Type::kGestureScrollUpdate:
    case blink::WebInputEvent::Type::kGestureScrollEnd:
      Emit("scroll-touch-edge");
      break;
    default:
      break;
  }
}

void BrowserWindow::RenderViewHostChanged(content::RenderViewHost* old_host,
                                          content::RenderViewHost* new_host) {
  if (old_host)
    old_host->GetWidget()->RemoveInputEventObserver(this);
  if (new_host)
    new_host->GetWidget()->AddInputEventObserver(this);
}

void BrowserWindow::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  if (!window()->transparent())
    return;

  content::RenderWidgetHostImpl* impl = content::RenderWidgetHostImpl::FromID(
      render_frame_host->GetProcess()->GetID(),
      render_frame_host->GetRoutingID());
  if (impl)
    impl->owner_delegate()->SetBackgroundOpaque(false);
}

void BrowserWindow::DidFirstVisuallyNonEmptyPaint() {
  if (window()->IsClosed() || window()->IsVisible())
    return;

  // 当存在非空的第一个绘制时，调整要强制的RenderWidget的大小。
  // 铬来画。
  auto* const view = web_contents()->GetRenderWidgetHostView();
  view->Show();
  view->SetSize(window()->GetContentSize());
}

void BrowserWindow::BeforeUnloadDialogCancelled() {
  WindowList::WindowCloseCancelled(window());
  // 取消窗口关闭时取消无响应事件。
  window_unresponsive_closure_.Cancel();
}

void BrowserWindow::OnRendererUnresponsive(content::RenderProcessHost*) {
  // 将无响应时间安排在稍后，因为我们可能会收到。
  // 很快就会有反应事件。这可能发生在整个应用程序。
  // 封锁了一段时间。
  // 另请注意，当关闭此事件时将被忽略，因为我们有。
  // 显式启动关闭超时计数器。这是故意的，因为我们。
  // 我不希望在用户关闭时过早发送无响应事件。
  // 窗户。
  ScheduleUnresponsiveEvent(50);
}

void BrowserWindow::WebContentsDestroyed() {
  api_web_contents_ = nullptr;
  CloseImmediately();
}

void BrowserWindow::OnCloseContents() {
  BaseWindow::ResetBrowserViews();
  api_web_contents_->Destroy();
}

void BrowserWindow::OnRendererResponsive(content::RenderProcessHost*) {
  window_unresponsive_closure_.Cancel();
  Emit("responsive");
}

void BrowserWindow::OnDraggableRegionsUpdated(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  UpdateDraggableRegions(regions);
}

void BrowserWindow::OnSetContentBounds(const gfx::Rect& rect) {
  // Window.size to(...)。
  // Window.moveto(...)。
  window()->SetBounds(rect, false);
}

void BrowserWindow::OnActivateContents() {
  // 聚焦WebContents时隐藏自动隐藏菜单。
#if !defined(OS_MAC)
  if (IsMenuBarAutoHide() && IsMenuBarVisible())
    window()->SetMenuBarVisibility(false);
#endif
}

void BrowserWindow::OnPageTitleUpdated(const std::u16string& title,
                                       bool explicit_set) {
  // 将窗口标题更改为页面标题。
  auto self = GetWeakPtr();
  if (!Emit("page-title-updated", title, explicit_set)) {
    // |此|可能已删除，或被Close()标记为销毁。
    if (self && !IsDestroyed())
      SetTitle(base::UTF16ToUTF8(title));
  }
}

void BrowserWindow::RequestPreferredWidth(int* width) {
  *width = web_contents()->GetPreferredSize().width();
}

void BrowserWindow::OnCloseButtonClicked(bool* prevent_default) {
  // 当用户尝试通过单击Close按钮关闭窗口时，我们会这样做。
  // 不是立即关闭窗口，而是尝试关闭网页。
  // 首先，当网页关闭时，窗口也将关闭。
  *prevent_default = true;

  // 假设窗口没有响应，如果它没有取消关闭，并且。
  // 没有在5秒内关闭，这样我们就可以快速地显示无响应的。
  // 当窗口忙于执行某些脚本而不等待。
  // 无响应超时。
  if (window_unresponsive_closure_.IsCancelled())
    ScheduleUnresponsiveEvent(5000);

  // 已由渲染器关闭。
  if (!web_contents())
    return;

  // 需要使Bere Unload处理程序工作。
  api_web_contents_->NotifyUserActivation();

  // 在关联的BrowserViews的Unload事件之前触发。
  for (NativeBrowserView* view : window_->browser_views()) {
    auto* vwc = view->web_contents();
    auto* api_web_contents = api::WebContents::From(vwc);

    // 需要使Bere Unload处理程序工作。
    if (api_web_contents)
      api_web_contents->NotifyUserActivation();

    if (vwc) {
      if (vwc->NeedToFireBeforeUnloadOrUnloadEvents()) {
        vwc->DispatchBeforeUnload(false /* 自动取消(_C)。*/);
      }
    }
  }

  if (web_contents()->NeedToFireBeforeUnloadOrUnloadEvents()) {
    web_contents()->DispatchBeforeUnload(false /* 自动取消(_C)。*/);
  } else {
    web_contents()->Close();
  }
}  // 命名空间API。

void BrowserWindow::OnWindowBlur() {
  if (api_web_contents_)
    web_contents()->StoreFocus();

  BaseWindow::OnWindowBlur();
}

void BrowserWindow::OnWindowFocus() {
  // 关闭窗口时可能会发出聚焦/模糊事件。
  if (api_web_contents_) {
    web_contents()->RestoreFocus();
#if !defined(OS_MAC)
    if (!api_web_contents_->IsDevToolsOpened())
      web_contents()->Focus();
#endif
  }

  BaseWindow::OnWindowFocus();
}

void BrowserWindow::OnWindowIsKeyChanged(bool is_key) {
#if defined(OS_MAC)
  auto* rwhv = web_contents()->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetActive(is_key);
  window()->SetActive(is_key);
#endif
}

void BrowserWindow::OnWindowResize() {
#if defined(OS_MAC)
  if (!draggable_regions_.empty()) {
    UpdateDraggableRegions(draggable_regions_);
  } else {
    for (NativeBrowserView* view : window_->browser_views()) {
      view->UpdateDraggableRegions(view->GetDraggableRegions());
    }
  }
#endif
  BaseWindow::OnWindowResize();
}

void BrowserWindow::OnWindowLeaveFullScreen() {
#if defined(OS_MAC)
  if (web_contents()->IsFullscreen())
    web_contents()->ExitFullscreen(true);
#endif
  BaseWindow::OnWindowLeaveFullScreen();
}

void BrowserWindow::UpdateWindowControlsOverlay(
    const gfx::Rect& bounding_rect) {
  web_contents()->UpdateWindowControlsOverlay(bounding_rect);
}

void BrowserWindow::CloseImmediately() {
  // 在关闭当前窗口之前关闭所有子窗口。
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  for (v8::Local<v8::Value> value : child_windows_.Values(isolate())) {
    gin::Handle<BrowserWindow> child;
    if (gin::ConvertFromV8(isolate(), value, &child) && !child.IsEmpty())
      child->window()->CloseImmediately();
  }

  BaseWindow::CloseImmediately();

  // 窗口关闭后不发送“无响应”事件。
  window_unresponsive_closure_.Cancel();
}

void BrowserWindow::Focus() {
  if (api_web_contents_->IsOffScreen())
    FocusOnWebView();
  else
    BaseWindow::Focus();
}

void BrowserWindow::Blur() {
  if (api_web_contents_->IsOffScreen())
    BlurWebView();
  else
    BaseWindow::Blur();
}

void BrowserWindow::SetBackgroundColor(const std::string& color_name) {
  BaseWindow::SetBackgroundColor(color_name);
  SkColor color = ParseHexColor(color_name);
  web_contents()->SetPageBaseBackgroundColor(color);
  auto* rwhv = web_contents()->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetBackgroundColor(color);
  // 还要更新Web首选项对象，否则视图将重置为。
  // 下一次加载URL调用。
  if (api_web_contents_) {
    auto* web_preferences =
        WebContentsPreferences::From(api_web_contents_->web_contents());
    if (web_preferences) {
      web_preferences->SetBackgroundColor(ParseHexColor(color_name));
    }
  }
}

void BrowserWindow::SetBrowserView(v8::Local<v8::Value> value) {
  BaseWindow::ResetBrowserViews();
  BaseWindow::AddBrowserView(value);
#if defined(OS_MAC)
  UpdateDraggableRegions(draggable_regions_);
#endif
}

void BrowserWindow::AddBrowserView(v8::Local<v8::Value> value) {
  BaseWindow::AddBrowserView(value);
#if defined(OS_MAC)
  UpdateDraggableRegions(draggable_regions_);
#endif
}

void BrowserWindow::RemoveBrowserView(v8::Local<v8::Value> value) {
  BaseWindow::RemoveBrowserView(value);
#if defined(OS_MAC)
  UpdateDraggableRegions(draggable_regions_);
#endif
}

void BrowserWindow::SetTopBrowserView(v8::Local<v8::Value> value,
                                      gin_helper::Arguments* args) {
  BaseWindow::SetTopBrowserView(value, args);
#if defined(OS_MAC)
  UpdateDraggableRegions(draggable_regions_);
#endif
}

void BrowserWindow::ResetBrowserViews() {
  BaseWindow::ResetBrowserViews();
#if defined(OS_MAC)
  UpdateDraggableRegions(draggable_regions_);
#endif
}

void BrowserWindow::OnDevToolsResized() {
  UpdateDraggableRegions(draggable_regions_);
}

void BrowserWindow::SetVibrancy(v8::Isolate* isolate,
                                v8::Local<v8::Value> value) {
  std::string type = gin::V8ToString(isolate, value);

  auto* render_view_host = web_contents()->GetRenderViewHost();
  if (render_view_host) {
    auto* impl = content::RenderWidgetHostImpl::FromID(
        render_view_host->GetProcess()->GetID(),
        render_view_host->GetRoutingID());
    if (impl)
      impl->owner_delegate()->SetBackgroundOpaque(
          type.empty() ? !window_->transparent() : false);
  }

  BaseWindow::SetVibrancy(isolate, value);
}

void BrowserWindow::FocusOnWebView() {
  web_contents()->GetRenderViewHost()->GetWidget()->Focus();
}

void BrowserWindow::BlurWebView() {
  web_contents()->GetRenderViewHost()->GetWidget()->Blur();
}

bool BrowserWindow::IsWebViewFocused() {
  auto* host_view = web_contents()->GetRenderViewHost()->GetWidget()->GetView();
  return host_view && host_view->HasFocus();
}

v8::Local<v8::Value> BrowserWindow::GetWebContents(v8::Isolate* isolate) {
  if (web_contents_.IsEmpty())
    return v8::Null(isolate);
  return v8::Local<v8::Value>::New(isolate, web_contents_);
}

void BrowserWindow::ScheduleUnresponsiveEvent(int ms) {
  if (!window_unresponsive_closure_.IsCancelled())
    return;

  window_unresponsive_closure_.Reset(base::BindRepeating(
      &BrowserWindow::NotifyWindowUnresponsive, GetWeakPtr()));
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, window_unresponsive_closure_.callback(),
      base::TimeDelta::FromMilliseconds(ms));
}

void BrowserWindow::NotifyWindowUnresponsive() {
  window_unresponsive_closure_.Cancel();
  if (!window_->IsClosed() && window_->IsEnabled() &&
      !IsUnresponsiveEventSuppressed()) {
    Emit("unresponsive");
  }
}

void BrowserWindow::OnWindowShow() {
  web_contents()->WasShown();
  BaseWindow::OnWindowShow();
}

void BrowserWindow::OnWindowHide() {
  web_contents()->WasOccluded();
  BaseWindow::OnWindowHide();
}

// 静电。
gin_helper::WrappableBase* BrowserWindow::New(gin_helper::ErrorThrower thrower,
                                              gin::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    thrower.ThrowError("Cannot create BrowserWindow before app is ready");
    return nullptr;
  }

  if (args->Length() > 1) {
    args->ThrowError();
    return nullptr;
  }

  gin_helper::Dictionary options;
  if (!(args->Length() == 1 && args->GetNext(&options))) {
    options = gin::Dictionary::CreateEmpty(args->isolate());
  }

  return new BrowserWindow(args, options);
}

// 静电。
void BrowserWindow::BuildPrototype(v8::Isolate* isolate,
                                   v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "BrowserWindow"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("focusOnWebView", &BrowserWindow::FocusOnWebView)
      .SetMethod("blurWebView", &BrowserWindow::BlurWebView)
      .SetMethod("isWebViewFocused", &BrowserWindow::IsWebViewFocused)
      .SetProperty("webContents", &BrowserWindow::GetWebContents);
}

// 静电。
v8::Local<v8::Value> BrowserWindow::From(v8::Isolate* isolate,
                                         NativeWindow* native_window) {
  auto* existing = TrackableObject::FromWrappedClass(isolate, native_window);
  if (existing)
    return existing->GetWrapper();
  else
    return v8::Null(isolate);
}

}  // 命名空间API。

}  // 命名空间电子。

namespace {

using electron::api::BrowserWindow;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("BrowserWindow",
           gin_helper::CreateConstructor<BrowserWindow>(
               isolate, base::BindRepeating(&BrowserWindow::New)));
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_window, Initialize)
