// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_H_
#define SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "chrome/browser/devtools/devtools_eye_dropper.h"
#include "chrome/browser/devtools/devtools_file_system_indexer.h"
#include "content/common/cursors/webcursor.h"
#include "content/common/frame.mojom.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/keyboard_event_processing_result.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "electron/buildflags/buildflags.h"
#include "electron/shell/common/api/api.mojom.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "printing/buildflags/buildflags.h"
#include "shell/browser/api/frame_subscriber.h"
#include "shell/browser/api/save_page_handler.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/extended_web_contents_observer.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_delegate.h"
#include "shell/browser/ui/inspectable_web_contents_view_delegate.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/pinnable.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/image/image.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "shell/browser/printing/print_preview_message_handler.h"
#include "shell/browser/printing/print_view_manager_electron.h"
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/common/mojom/view_type.mojom.h"

namespace extensions {
class ScriptExecutor;
}
#endif

namespace blink {
struct DeviceEmulationParams;
}

namespace gin_helper {
class Dictionary;
}

namespace network {
class ResourceRequestBody;
}

namespace gin {
class Arguments;
}

namespace electron {

class ElectronBrowserContext;
class ElectronJavaScriptDialogManager;
class InspectableWebContents;
class WebContentsZoomController;
class WebViewGuestDelegate;
class FrameSubscriber;
class WebDialogHelper;
class NativeWindow;

#if BUILDFLAG(ENABLE_OSR)
class OffScreenRenderWidgetHostView;
class OffScreenWebContentsView;
#endif

namespace api {

using DevicePermissionMap = std::map<
    int,
    std::map<content::PermissionType,
             std::map<url::Origin, std::vector<std::unique_ptr<base::Value>>>>>;

// 对Content：：WebContents进行包装。
class WebContents : public gin::Wrappable<WebContents>,
                    public gin_helper::EventEmitterMixin<WebContents>,
                    public gin_helper::Constructible<WebContents>,
                    public gin_helper::Pinnable<WebContents>,
                    public gin_helper::CleanedUpAtExit,
                    public content::WebContentsObserver,
                    public content::WebContentsDelegate,
                    public InspectableWebContentsDelegate,
                    public InspectableWebContentsViewDelegate {
 public:
  enum class Type {
    kBackgroundPage,  // 扩展背景页。
    kBrowserWindow,   // 由BrowserWindow使用。
    kBrowserView,     // 由BrowserView使用。
    kRemote,          // 将现有的WebContents薄薄地包裹起来。
    kWebView,         // 由&lt;webview&gt;使用。
    kOffScreen,       // 用于屏幕外渲染。
  };

  // 创建一个新的WebContents并返回它的V8包装器。
  static gin::Handle<WebContents> New(v8::Isolate* isolate,
                                      const gin_helper::Dictionary& options);

  // 为现有|web_content|创建新的V8包装器。
  // 
  // |web_content|的生存期将由此类管理。
  static gin::Handle<WebContents> CreateAndTake(
      v8::Isolate* isolate,
      std::unique_ptr<content::WebContents> web_contents,
      Type type);

  // 获取|web_Contents|关联的API：：WebContents。返回nullptr。
  // 如果没有关联的包装器。
  static WebContents* From(content::WebContents* web_contents);
  static WebContents* FromID(int32_t id);

  // 获取|web_content|的V8包装器，如果不存在，则创建一个。
  // 
  // |web_content|的生存期不受此类管理，并且类型。
  // 这个包装器的属性总是远程的。
  static gin::Handle<WebContents> FromOrCreate(
      v8::Isolate* isolate,
      content::WebContents* web_contents);

  static gin::Handle<WebContents> CreateFromWebPreferences(
      v8::Isolate* isolate,
      const gin_helper::Dictionary& web_preferences);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
      v8::Isolate*,
      v8::Local<v8::ObjectTemplate>);
  const char* GetTypeName() override;

  void Destroy();
  base::WeakPtr<WebContents> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

  bool GetBackgroundThrottling() const;
  void SetBackgroundThrottling(bool allowed);
  int GetProcessID() const;
  base::ProcessId GetOSProcessID() const;
  Type GetType() const;
  bool Equal(const WebContents* web_contents) const;
  void LoadURL(const GURL& url, const gin_helper::Dictionary& options);
  void Reload();
  void ReloadIgnoringCache();
  void DownloadURL(const GURL& url);
  GURL GetURL() const;
  std::u16string GetTitle() const;
  bool IsLoading() const;
  bool IsLoadingMainFrame() const;
  bool IsWaitingForResponse() const;
  void Stop();
  bool CanGoBack() const;
  void GoBack();
  bool CanGoForward() const;
  void GoForward();
  bool CanGoToOffset(int offset) const;
  void GoToOffset(int offset);
  bool CanGoToIndex(int index) const;
  void GoToIndex(int index);
  int GetActiveIndex() const;
  void ClearHistory();
  int GetHistoryLength() const;
  const std::string GetWebRTCIPHandlingPolicy() const;
  void SetWebRTCIPHandlingPolicy(const std::string& webrtc_ip_handling_policy);
  bool IsCrashed() const;
  void ForcefullyCrashRenderer();
  void SetUserAgent(const std::string& user_agent);
  std::string GetUserAgent();
  void InsertCSS(const std::string& css);
  v8::Local<v8::Promise> SavePage(const base::FilePath& full_file_path,
                                  const content::SavePageType& save_type);
  void OpenDevTools(gin::Arguments* args);
  void CloseDevTools();
  bool IsDevToolsOpened();
  bool IsDevToolsFocused();
  void ToggleDevTools();
  void EnableDeviceEmulation(const blink::DeviceEmulationParams& params);
  void DisableDeviceEmulation();
  void InspectElement(int x, int y);
  void InspectSharedWorker();
  void InspectSharedWorkerById(const std::string& workerId);
  std::vector<scoped_refptr<content::DevToolsAgentHost>> GetAllSharedWorkers();
  void InspectServiceWorker();
  void SetIgnoreMenuShortcuts(bool ignore);
  void SetAudioMuted(bool muted);
  bool IsAudioMuted();
  bool IsCurrentlyAudible();
  void SetEmbedder(const WebContents* embedder);
  void SetDevToolsWebContents(const WebContents* devtools);
  v8::Local<v8::Value> GetNativeView(v8::Isolate* isolate) const;
  void IncrementCapturerCount(gin::Arguments* args);
  void DecrementCapturerCount(gin::Arguments* args);
  bool IsBeingCaptured();
  void HandleNewRenderFrame(content::RenderFrameHost* render_frame_host);

#if BUILDFLAG(ENABLE_PRINTING)
  void OnGetDefaultPrinter(base::Value print_settings,
                           printing::CompletionCallback print_callback,
                           std::u16string device_name,
                           bool silent,
                           // &lt;ERROR，DEFAULT_PRINTER_NAME&gt;。
                           std::pair<std::string, std::u16string> info);
  void Print(gin::Arguments* args);
  // 将当前页面打印为PDF。
  v8::Local<v8::Promise> PrintToPDF(base::DictionaryValue settings);
#endif

  void SetNextChildWebPreferences(const gin_helper::Dictionary);

  // DevTools工作区API。
  void AddWorkSpace(gin::Arguments* args, const base::FilePath& path);
  void RemoveWorkSpace(gin::Arguments* args, const base::FilePath& path);

  // 编辑命令。
  void Undo();
  void Redo();
  void Cut();
  void Copy();
  void Paste();
  void PasteAndMatchStyle();
  void Delete();
  void SelectAll();
  void Unselect();
  void Replace(const std::u16string& word);
  void ReplaceMisspelling(const std::u16string& word);
  uint32_t FindInPage(gin::Arguments* args);
  void StopFindInPage(content::StopFindAction action);
  void ShowDefinitionForSelection();
  void CopyImageAt(int x, int y);

  // 集中注意力。
  void Focus();
  bool IsFocused() const;

  // 将WebInputEvent发送到页面。
  void SendInputEvent(v8::Isolate* isolate, v8::Local<v8::Value> input_event);

  // 订阅帧更新。
  void BeginFrameSubscription(gin::Arguments* args);
  void EndFrameSubscription();

  // 拖动本机项目。
  void StartDrag(const gin_helper::Dictionary& item, gin::Arguments* args);

  // 使用|RECT|捕获页面，捕获时将调用|Callback|。
  // 搞定了。
  v8::Local<v8::Promise> CapturePage(gin::Arguments* args);

  // 创建&lt;webview&gt;的方法。
  bool IsGuest() const;
  void AttachToIframe(content::WebContents* embedder_web_contents,
                      int embedder_frame_id);
  void DetachFromOuterFrame();

  // 屏幕外渲染的方法。
  bool IsOffScreen() const;
#if BUILDFLAG(ENABLE_OSR)
  void OnPaint(const gfx::Rect& dirty_rect, const SkBitmap& bitmap);
  void StartPainting();
  void StopPainting();
  bool IsPainting() const;
  void SetFrameRate(int frame_rate);
  int GetFrameRate() const;
#endif
  void Invalidate();
  gfx::Size GetSizeForNewRenderView(content::WebContents*) override;

  // 缩放处理的方法。
  void SetZoomLevel(double level);
  double GetZoomLevel() const;
  void SetZoomFactor(gin_helper::ErrorThrower thrower, double factor);
  double GetZoomFactor() const;

  // 权限响应触发回调。
  void OnEnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame,
      const blink::mojom::FullscreenOptions& options,
      bool allowed);

  // 使用给定的配置创建窗口。
  void OnCreateWindow(const GURL& target_url,
                      const content::Referrer& referrer,
                      const std::string& frame_name,
                      WindowOpenDisposition disposition,
                      const std::string& features,
                      const scoped_refptr<network::ResourceRequestBody>& body);

  // 返回当前WebContents的预加载脚本路径。
  std::vector<base::FilePath> GetPreloadPaths() const;

  // 返回当前WebContents的Web首选项。
  v8::Local<v8::Value> GetWebPreferences(v8::Isolate* isolate) const;
  v8::Local<v8::Value> GetLastWebPreferences(v8::Isolate* isolate) const;

  // 返回所有者窗口。
  v8::Local<v8::Value> GetOwnerBrowserWindow(v8::Isolate* isolate) const;

  // 通知网页存在用户交互。
  void NotifyUserActivation();

  v8::Local<v8::Promise> TakeHeapSnapshot(v8::Isolate* isolate,
                                          const base::FilePath& file_path);
  v8::Local<v8::Promise> GetProcessMemoryInfo(v8::Isolate* isolate);

  // 财产。
  int32_t ID() const { return id_; }
  v8::Local<v8::Value> Session(v8::Isolate* isolate);
  content::WebContents* HostWebContents() const;
  v8::Local<v8::Value> DevToolsWebContents(v8::Isolate* isolate);
  v8::Local<v8::Value> Debugger(v8::Isolate* isolate);
  content::RenderFrameHost* MainFrame();

  WebContentsZoomController* GetZoomController() { return zoom_controller_; }

  void AddObserver(ExtendedWebContentsObserver* obs) {
    observers_.AddObserver(obs);
  }
  void RemoveObserver(ExtendedWebContentsObserver* obs) {
    // 尝试从空集合中移除会导致访问冲突。
    if (!observers_.empty())
      observers_.RemoveObserver(obs);
  }

  bool EmitNavigationEvent(const std::string& event,
                           content::NavigationHandle* navigation_handle);

  // This.emit(名称，新事件(发送者，消息)，参数...)；
  template <typename... Args>
  bool EmitWithSender(base::StringPiece name,
                      content::RenderFrameHost* sender,
                      electron::mojom::ElectronBrowser::InvokeCallback callback,
                      Args&&... args) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Object> wrapper;
    if (!GetWrapper(isolate).ToLocal(&wrapper))
      return false;
    v8::Local<v8::Object> event = gin_helper::internal::CreateNativeEvent(
        isolate, wrapper, sender, std::move(callback));
    return EmitCustomEvent(name, event, std::forward<Args>(args)...);
  }

  WebContents* embedder() { return embedder_; }

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions::ScriptExecutor* script_executor() {
    return script_executor_.get();
  }
#endif

  // 将该窗口设置为所有者窗口。
  void SetOwnerWindow(NativeWindow* owner_window);
  void SetOwnerWindow(content::WebContents* web_contents,
                      NativeWindow* owner_window);

  // 返回此委托管理的WebContents。
  content::WebContents* GetWebContents() const;

  // 返回DevTools的WebContents。
  content::WebContents* GetDevToolsWebContents() const;

  InspectableWebContents* inspectable_web_contents() const {
    return inspectable_web_contents_.get();
  }

  NativeWindow* owner_window() const { return owner_window_.get(); }

  bool is_html_fullscreen() const { return html_fullscreen_; }

  void set_fullscreen_frame(content::RenderFrameHost* rfh) {
    fullscreen_frame_ = rfh;
  }

  // Mojom：：电子浏览器。
  void Message(bool internal,
               const std::string& channel,
               blink::CloneableMessage arguments,
               content::RenderFrameHost* render_frame_host);
  void Invoke(bool internal,
              const std::string& channel,
              blink::CloneableMessage arguments,
              electron::mojom::ElectronBrowser::InvokeCallback callback,
              content::RenderFrameHost* render_frame_host);
  void OnFirstNonEmptyLayout(content::RenderFrameHost* render_frame_host);
  void ReceivePostMessage(const std::string& channel,
                          blink::TransferableMessage message,
                          content::RenderFrameHost* render_frame_host);
  void MessageSync(
      bool internal,
      const std::string& channel,
      blink::CloneableMessage arguments,
      electron::mojom::ElectronBrowser::MessageSyncCallback callback,
      content::RenderFrameHost* render_frame_host);
  void MessageTo(int32_t web_contents_id,
                 const std::string& channel,
                 blink::CloneableMessage arguments);
  void MessageHost(const std::string& channel,
                   blink::CloneableMessage arguments,
                   content::RenderFrameHost* render_frame_host);
  void UpdateDraggableRegions(std::vector<mojom::DraggableRegionPtr> regions);
  void SetTemporaryZoomLevel(double level);
  void DoGetZoomLevel(
      electron::mojom::ElectronBrowser::DoGetZoomLevelCallback callback);
  void SetImageAnimationPolicy(const std::string& new_policy);

  // 授予|源|访问|设备|的权限。
  // 将取代ObjectPermissionContextBase：：GrantObjectPermission.使用。
  void GrantDevicePermission(const url::Origin& origin,
                             const base::Value* device,
                             content::PermissionType permissionType,
                             content::RenderFrameHost* render_frame_host);

  // 返回|Origin|已被授予权限的设备列表。
  // 进入。将被用来代替。
  // ObjectPermissionContextBase：：GetGrantedObjects.。
  std::vector<base::Value> GetGrantedDevices(
      const url::Origin& origin,
      content::PermissionType permissionType,
      content::RenderFrameHost* render_frame_host);

 private:
  // 不管理|WEB_CONTENTS|的生存期。
  WebContents(v8::Isolate* isolate, content::WebContents* web_contents);
  // 接管|web_content|的所有权。
  WebContents(v8::Isolate* isolate,
              std::unique_ptr<content::WebContents> web_contents,
              Type type);
  // 创建新的Content：：WebContents。
  WebContents(v8::Isolate* isolate, const gin_helper::Dictionary& options);
  ~WebContents() override;

  // 如果垃圾收集尚未开始，请删除此选项。
  void DeleteThisIfAlive();

  // 创建InspectableWebContents对象并取得。
  // |web_Contents|。
  void InitWithWebContents(std::unique_ptr<content::WebContents> web_contents,
                           ElectronBrowserContext* browser_context,
                           bool is_guest);

  void InitWithSessionAndOptions(
      v8::Isolate* isolate,
      std::unique_ptr<content::WebContents> web_contents,
      gin::Handle<class Session> session,
      const gin_helper::Dictionary& options);

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  void InitWithExtensionView(v8::Isolate* isolate,
                             content::WebContents* web_contents,
                             extensions::mojom::ViewType view_type);
#endif

  // 内容：：WebContentsDelegate：
  bool DidAddMessageToConsole(content::WebContents* source,
                              blink::mojom::ConsoleMessageLevel level,
                              const std::u16string& message,
                              int32_t line_no,
                              const std::u16string& source_id) override;
  bool IsWebContentsCreationOverridden(
      content::SiteInstance* source_site_instance,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const content::mojom::CreateNewWindowParams& params) override;
  content::WebContents* CreateCustomWebContents(
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      bool is_new_browsing_instance,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url,
      const content::StoragePartitionId& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;
  void WebContentsCreatedWithFullParams(
      content::WebContents* source_contents,
      int opener_render_process_id,
      int opener_render_frame_id,
      const content::mojom::CreateNewWindowParams& params,
      content::WebContents* new_contents) override;
  void AddNewContents(content::WebContents* source,
                      std::unique_ptr<content::WebContents> new_contents,
                      const GURL& target_url,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override;
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  void BeforeUnloadFired(content::WebContents* tab,
                         bool proceed,
                         bool* proceed_to_fire_unload) override;
  void SetContentsBounds(content::WebContents* source,
                         const gfx::Rect& pos) override;
  void CloseContents(content::WebContents* source) override;
  void ActivateContents(content::WebContents* contents) override;
  void UpdateTargetURL(content::WebContents* source, const GURL& url) override;
  bool HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override;
  bool PlatformHandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event);
  content::KeyboardEventProcessingResult PreHandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override;
  void ContentsZoomChange(bool zoom_in) override;
  void EnterFullscreenModeForTab(
      content::RenderFrameHost* requesting_frame,
      const blink::mojom::FullscreenOptions& options) override;
  void ExitFullscreenModeForTab(content::WebContents* source) override;
  void RendererUnresponsive(
      content::WebContents* source,
      content::RenderWidgetHost* render_widget_host,
      base::RepeatingClosure hang_monitor_restarter) override;
  void RendererResponsive(
      content::WebContents* source,
      content::RenderWidgetHost* render_widget_host) override;
  bool HandleContextMenu(content::RenderFrameHost* render_frame_host,
                         const content::ContextMenuParams& params) override;
  bool OnGoToEntryOffset(int offset) override;
  void FindReply(content::WebContents* web_contents,
                 int request_id,
                 int number_of_matches,
                 const gfx::Rect& selection_rect,
                 int active_match_ordinal,
                 bool final_update) override;
  bool CheckMediaAccessPermission(content::RenderFrameHost* render_frame_host,
                                  const GURL& security_origin,
                                  blink::mojom::MediaStreamType type) override;
  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      content::MediaResponseCallback callback) override;
  void RequestToLockMouse(content::WebContents* web_contents,
                          bool user_gesture,
                          bool last_unlocked_by_target) override;
  content::JavaScriptDialogManager* GetJavaScriptDialogManager(
      content::WebContents* source) override;
  void OnAudioStateChanged(bool audible) override;
  void UpdatePreferredSize(content::WebContents* web_contents,
                           const gfx::Size& pref_size) override;

  // 内容：：WebContentsViewer：
  void BeforeUnloadFired(bool proceed,
                         const base::TimeTicks& proceed_time) override;
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;
  void FrameDeleted(int frame_tree_node_id) override;
  void RenderViewDeleted(content::RenderViewHost*) override;
  void RenderProcessGone(base::TerminationStatus status) override;
  void DOMContentLoaded(content::RenderFrameHost* render_frame_host) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code) override;
  void DidStartLoading() override;
  void DidStopLoading() override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidRedirectNavigation(
      content::NavigationHandle* navigation_handle) override;
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void DidUpdateFaviconURL(
      content::RenderFrameHost* render_frame_host,
      const std::vector<blink::mojom::FaviconURLPtr>& urls) override;
  void PluginCrashed(const base::FilePath& plugin_path,
                     base::ProcessId plugin_pid) override;
  void MediaStartedPlaying(const MediaPlayerInfo& video_type,
                           const content::MediaPlayerId& id) override;
  void MediaStoppedPlaying(
      const MediaPlayerInfo& video_type,
      const content::MediaPlayerId& id,
      content::WebContentsObserver::MediaStoppedReason reason) override;
  void DidChangeThemeColor() override;
  void OnCursorChanged(const content::WebCursor& cursor) override;
  void DidAcquireFullscreen(content::RenderFrameHost* rfh) override;

  // InspectableWebContentsDelegate：
  void DevToolsReloadPage() override;

  // InspectableWebContentsViewDelegate：
  void DevToolsFocused() override;
  void DevToolsOpened() override;
  void DevToolsClosed() override;
  void DevToolsResized() override;

  ElectronBrowserContext* GetBrowserContext() const;

  void OnElectronBrowserConnectionError();

#if BUILDFLAG(ENABLE_OSR)
  OffScreenWebContentsView* GetOffScreenWebContentsView() const;
  OffScreenRenderWidgetHostView* GetOffScreenRenderWidgetHostView() const;
#endif

  // 在接收到来自呈现器的同步消息时调用。
  // 获取缩放级别。
  void OnGetZoomLevel(content::RenderFrameHost* frame_host,
                      IPC::Message* reply_msg);

  void InitZoomController(content::WebContents* web_contents,
                          const gin_helper::Dictionary& options);

  // 内容：：WebContentsDelegate：
  bool CanOverscrollContent() override;
  std::unique_ptr<content::EyeDropper> OpenEyeDropper(
      content::RenderFrameHost* frame,
      content::EyeDropperListener* listener) override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      scoped_refptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override;
  void EnumerateDirectory(content::WebContents* web_contents,
                          scoped_refptr<content::FileSelectListener> listener,
                          const base::FilePath& path) override;
  bool IsFullscreenForTabOrPending(const content::WebContents* source) override;
  blink::SecurityStyle GetSecurityStyle(
      content::WebContents* web_contents,
      content::SecurityStyleExplanations* explanations) override;
  bool TakeFocus(content::WebContents* source, bool reverse) override;
  content::PictureInPictureResult EnterPictureInPicture(
      content::WebContents* web_contents,
      const viz::SurfaceId&,
      const gfx::Size& natural_size) override;
  void ExitPictureInPicture() override;

  // InspectableWebContentsDelegate：
  void DevToolsSaveToFile(const std::string& url,
                          const std::string& content,
                          bool save_as) override;
  void DevToolsAppendToFile(const std::string& url,
                            const std::string& content) override;
  void DevToolsRequestFileSystems() override;
  void DevToolsAddFileSystem(const std::string& type,
                             const base::FilePath& file_system_path) override;
  void DevToolsRemoveFileSystem(
      const base::FilePath& file_system_path) override;
  void DevToolsIndexPath(int request_id,
                         const std::string& file_system_path,
                         const std::string& excluded_folders_message) override;
  void DevToolsStopIndexing(int request_id) override;
  void DevToolsSearchInPath(int request_id,
                            const std::string& file_system_path,
                            const std::string& query) override;
  void DevToolsSetEyeDropperActive(bool active) override;

  // InspectableWebContentsViewDelegate：
#if defined(TOOLKIT_VIEWS) && !defined(OS_MAC)
  ui::ImageModel GetDevToolsWindowIcon() override;
#endif
#if defined(OS_LINUX)
  void GetDevToolsWindowWMClass(std::string* name,
                                std::string* class_name) override;
#endif

  void ColorPickedInEyeDropper(int r, int g, int b, int a);

  // DevTools索引事件回调。
  void OnDevToolsIndexingWorkCalculated(int request_id,
                                        const std::string& file_system_path,
                                        int total_work);
  void OnDevToolsIndexingWorked(int request_id,
                                const std::string& file_system_path,
                                int worked);
  void OnDevToolsIndexingDone(int request_id,
                              const std::string& file_system_path);
  void OnDevToolsSearchCompleted(int request_id,
                                 const std::string& file_system_path,
                                 const std::vector<std::string>& file_paths);

  // 设置html接口触发的全屏模式。
  void SetHtmlApiFullscreen(bool enter_fullscreen);
  // 在浏览器和渲染器中更新html全屏标志。
  void UpdateHtmlApiFullscreen(bool fullscreen);

  v8::Global<v8::Value> session_;
  v8::Global<v8::Value> devtools_web_contents_;
  v8::Global<v8::Value> debugger_;

  std::unique_ptr<ElectronJavaScriptDialogManager> dialog_manager_;
  std::unique_ptr<WebViewGuestDelegate> guest_delegate_;
  std::unique_ptr<FrameSubscriber> frame_subscriber_;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  std::unique_ptr<extensions::ScriptExecutor> script_executor_;
#endif

  // 可能包含此Web内容的宿主Web内容。
  WebContents* embedder_ = nullptr;

  // 是否已附加来宾视图。
  bool attached_ = false;

  // 此webContents的缩放控制器。
  WebContentsZoomController* zoom_controller_ = nullptr;

  // 当前WebContents的类型。
  Type type_ = Type::kBrowserWindow;

  int32_t id_;

  // 用于findInPage请求的请求ID。
  uint32_t find_in_page_request_id_ = 0;

  // 是否禁用后台限制。
  bool background_throttling_ = true;

  // 是否启用DevTools。
  bool enable_devtools_ = true;

  // 此Web内容的观察者。
  base::ObserverList<ExtendedWebContentsObserver> observers_;

  v8::Global<v8::Value> pending_child_web_preferences_;

  // 此WebContents所属的窗口。
  base::WeakPtr<NativeWindow> owner_window_;

  bool offscreen_ = false;

  // 是否使用HTML5 API对窗口进行全屏显示。
  bool html_fullscreen_ = false;

  // 窗口API是否对窗口进行全屏显示。
  bool native_fullscreen_ = false;

  scoped_refptr<DevToolsFileSystemIndexer> devtools_file_system_indexer_;

  std::unique_ptr<DevToolsEyeDropper> eye_dropper_;

  ElectronBrowserContext* browser_context_;

  // 存储的InspectableWebContents对象。
  // 请注意，检查表_Web_CONTENTS_必须放在。
  // DIALOG_MANAGER_，因此我们可以确保检查表_Web_CONTENTS_是。
  // 在DIALOG_MANAGER_之前销毁，否则将发生崩溃。
  std::unique_ptr<InspectableWebContents> inspectable_web_contents_;

  // 将url映射到文件路径，由DevTools发送的文件请求使用。
  typedef std::map<std::string, base::FilePath> PathsMap;
  PathsMap saved_files_;

  // 将ID映射到索引作业，用于来自DevTools的文件系统索引请求。
  typedef std::
      map<int, scoped_refptr<DevToolsFileSystemIndexer::FileSystemIndexingJob>>
          DevToolsIndexingJobsMap;
  DevToolsIndexingJobsMap devtools_indexing_jobs_;

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

#if BUILDFLAG(ENABLE_PRINTING)
  scoped_refptr<base::TaskRunner> print_task_runner_;
#endif

  // 存储当前在全屏中的帧，如果没有帧，则存储nullptr。
  content::RenderFrameHost* fullscreen_frame_ = nullptr;

  // 保存已被授予权限的对象的内存中缓存。
  DevicePermissionMap granted_devices_;

  base::WeakPtrFactory<WebContents> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(WebContents);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_H_
