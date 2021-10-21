// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_BROWSER_WINDOW_H_
#define SHELL_BROWSER_API_ELECTRON_API_BROWSER_WINDOW_H_

#include <string>
#include <vector>

#include "base/cancelable_callback.h"
#include "shell/browser/api/electron_api_base_window.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/ui/drag_util.h"
#include "shell/common/gin_helper/error_thrower.h"

namespace electron {

namespace api {

class BrowserWindow : public BaseWindow,
                      public content::RenderWidgetHost::InputEventObserver,
                      public content::WebContentsObserver,
                      public ExtendedWebContentsObserver {
 public:
  static gin_helper::WrappableBase* New(gin_helper::ErrorThrower thrower,
                                        gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // 从|NATIVE_WINDOW|返回BrowserWindow对象。
  static v8::Local<v8::Value> From(v8::Isolate* isolate,
                                   NativeWindow* native_window);

  base::WeakPtr<BrowserWindow> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 protected:
  BrowserWindow(gin::Arguments* args, const gin_helper::Dictionary& options);
  ~BrowserWindow() override;

  // Content：：RenderWidgetHost：：InputEventObserver：
  void OnInputEvent(const blink::WebInputEvent& event) override;

  // 内容：：WebContentsViewer：
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void DidFirstVisuallyNonEmptyPaint() override;
  void BeforeUnloadDialogCancelled() override;
  void OnRendererUnresponsive(content::RenderProcessHost*) override;
  void OnRendererResponsive(
      content::RenderProcessHost* render_process_host) override;
  void WebContentsDestroyed() override;

  // ExtendedWebContentsViewer：
  void OnCloseContents() override;
  void OnDraggableRegionsUpdated(
      const std::vector<mojom::DraggableRegionPtr>& regions) override;
  void OnSetContentBounds(const gfx::Rect& rect) override;
  void OnActivateContents() override;
  void OnPageTitleUpdated(const std::u16string& title,
                          bool explicit_set) override;
  void OnDevToolsResized() override;

  // NativeWindowViewer：
  void RequestPreferredWidth(int* width) override;
  void OnCloseButtonClicked(bool* prevent_default) override;
  void OnWindowIsKeyChanged(bool is_key) override;
  void UpdateWindowControlsOverlay(const gfx::Rect& bounding_rect) override;

  // BaseWindow：
  void OnWindowBlur() override;
  void OnWindowFocus() override;
  void OnWindowResize() override;
  void OnWindowLeaveFullScreen() override;
  void CloseImmediately() override;
  void Focus() override;
  void Blur() override;
  void SetBackgroundColor(const std::string& color_name) override;
  void SetBrowserView(v8::Local<v8::Value> value) override;
  void AddBrowserView(v8::Local<v8::Value> value) override;
  void RemoveBrowserView(v8::Local<v8::Value> value) override;
  void SetTopBrowserView(v8::Local<v8::Value> value,
                         gin_helper::Arguments* args) override;
  void ResetBrowserViews() override;
  void SetVibrancy(v8::Isolate* isolate, v8::Local<v8::Value> value) override;
  void OnWindowShow() override;
  void OnWindowHide() override;

  // BrowserWindow接口。
  void FocusOnWebView();
  void BlurWebView();
  bool IsWebViewFocused();
  v8::Local<v8::Value> GetWebContents(v8::Isolate* isolate);

 private:
#if defined(OS_MAC)
  void OverrideNSWindowContentView(InspectableWebContentsView* webView);
#endif

  // 帮手。

  // 当窗口需要更新其可拖动区域时调用。
  void UpdateDraggableRegions(
      const std::vector<mojom::DraggableRegionPtr>& regions);

  // 安排通知无响应事件。
  void ScheduleUnresponsiveEvent(int ms);

  // 向观察者发送无响应事件。
  void NotifyWindowUnresponsive();

  // 关闭时窗口无响应时将调用的关闭，
  // 当我们可以证明窗口有响应时，它应该被取消。
  base::CancelableRepeatingClosure window_unresponsive_closure_;

  std::vector<mojom::DraggableRegionPtr> draggable_regions_;

  v8::Global<v8::Value> web_contents_;
  base::WeakPtr<api::WebContents> api_web_contents_;

  base::WeakPtrFactory<BrowserWindow> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BrowserWindow);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_BROWSER_WINDOW_H_
