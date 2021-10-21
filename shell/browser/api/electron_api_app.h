// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_APP_H_
#define SHELL_BROWSER_API_ELECTRON_API_APP_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/icon_manager.h"
#include "chrome/browser/process_singleton.h"
#include "content/public/browser/browser_child_process_observer.h"
#include "content/public/browser/gpu_data_manager_observer.h"
#include "content/public/browser/render_process_host.h"
#include "gin/handle.h"
#include "net/base/completion_once_callback.h"
#include "net/base/completion_repeating_callback.h"
#include "net/ssl/client_cert_identity.h"
#include "shell/browser/api/process_metric.h"
#include "shell/browser/browser.h"
#include "shell/browser/browser_observer.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"

#if defined(USE_NSS_CERTS)
#include "chrome/browser/certificate_manager_model.h"
#endif

namespace base {
class FilePath;
}

namespace electron {

#if defined(OS_WIN)
enum class JumpListResult : int;
#endif

namespace api {

class App : public ElectronBrowserClient::Delegate,
            public gin::Wrappable<App>,
            public gin_helper::EventEmitterMixin<App>,
            public BrowserObserver,
            public content::GpuDataManagerObserver,
            public content::BrowserChildProcessObserver {
 public:
  using FileIconCallback =
      base::RepeatingCallback<void(v8::Local<v8::Value>, const gfx::Image&)>;

  static gin::Handle<App> Create(v8::Isolate* isolate);
  static App* Get();

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

#if defined(USE_NSS_CERTS)
  void OnCertificateManagerModelCreated(
      base::Value options,
      net::CompletionOnceCallback callback,
      std::unique_ptr<CertificateManagerModel> model);
#endif

  base::FilePath GetAppPath() const;
  void RenderProcessReady(content::RenderProcessHost* host);
  void RenderProcessExited(content::RenderProcessHost* host);

  static bool IsPackaged();

  App();

 private:
  ~App() override;

  // 浏览器观察者：
  void OnBeforeQuit(bool* prevent_default) override;
  void OnWillQuit(bool* prevent_default) override;
  void OnWindowAllClosed() override;
  void OnQuit() override;
  void OnOpenFile(bool* prevent_default, const std::string& file_path) override;
  void OnOpenURL(const std::string& url) override;
  void OnActivate(bool has_visible_windows) override;
  void OnWillFinishLaunching() override;
  void OnFinishLaunching(const base::DictionaryValue& launch_info) override;
  void OnAccessibilitySupportChanged() override;
  void OnPreMainMessageLoopRun() override;
  void OnPreCreateThreads() override;
#if defined(OS_MAC)
  void OnWillContinueUserActivity(bool* prevent_default,
                                  const std::string& type) override;
  void OnDidFailToContinueUserActivity(const std::string& type,
                                       const std::string& error) override;
  void OnContinueUserActivity(bool* prevent_default,
                              const std::string& type,
                              const base::DictionaryValue& user_info,
                              const base::DictionaryValue& details) override;
  void OnUserActivityWasContinued(
      const std::string& type,
      const base::DictionaryValue& user_info) override;
  void OnUpdateUserActivityState(
      bool* prevent_default,
      const std::string& type,
      const base::DictionaryValue& user_info) override;
  void OnNewWindowForTab() override;
  void OnDidBecomeActive() override;
#endif

  // 内容：：ContentBrowserClient：
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      bool is_main_frame_request,
      bool strict_enforcement,
      base::OnceCallback<void(content::CertificateRequestResultType)> callback)
      override;
  base::OnceClosure SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList identities,
      std::unique_ptr<content::ClientCertificateDelegate> delegate) override;
  bool CanCreateWindow(content::RenderFrameHost* opener,
                       const GURL& opener_url,
                       const GURL& opener_top_level_frame_url,
                       const url::Origin& source_origin,
                       content::mojom::WindowContainerType container_type,
                       const GURL& target_url,
                       const content::Referrer& referrer,
                       const std::string& frame_name,
                       WindowOpenDisposition disposition,
                       const blink::mojom::WindowFeatures& features,
                       const std::string& raw_features,
                       const scoped_refptr<network::ResourceRequestBody>& body,
                       bool user_gesture,
                       bool opener_suppressed,
                       bool* no_javascript_access) override;

  // 内容：：GpuDataManagerWatch：
  void OnGpuInfoUpdate() override;
  void OnGpuProcessCrashed(base::TerminationStatus status) override;

  // 内容：：BrowserChildProcessWatch：
  void BrowserChildProcessLaunchedAndConnected(
      const content::ChildProcessData& data) override;
  void BrowserChildProcessHostDisconnected(
      const content::ChildProcessData& data) override;
  void BrowserChildProcessCrashed(
      const content::ChildProcessData& data,
      const content::ChildProcessTerminationInfo& info) override;
  void BrowserChildProcessKilled(
      const content::ChildProcessData& data,
      const content::ChildProcessTerminationInfo& info) override;

 private:
  void BrowserChildProcessCrashedOrKilled(
      const content::ChildProcessData& data,
      const content::ChildProcessTerminationInfo& info);

  void SetAppPath(const base::FilePath& app_path);
  void ChildProcessLaunched(int process_type,
                            int pid,
                            base::ProcessHandle handle,
                            const std::string& service_name = std::string(),
                            const std::string& name = std::string());
  void ChildProcessDisconnected(int pid);

  void SetAppLogsPath(gin_helper::ErrorThrower thrower,
                      absl::optional<base::FilePath> custom_path);

  // 获取/设置PathService中的预定义路径。
  base::FilePath GetPath(gin_helper::ErrorThrower thrower,
                         const std::string& name);
  void SetPath(gin_helper::ErrorThrower thrower,
               const std::string& name,
               const base::FilePath& path);

  void SetDesktopName(const std::string& desktop_name);
  std::string GetLocale();
  std::string GetLocaleCountryCode();
  void OnSecondInstance(const base::CommandLine& cmd,
                        const base::FilePath& cwd,
                        const std::vector<const uint8_t> additional_data);
  bool HasSingleInstanceLock() const;
  bool RequestSingleInstanceLock(gin::Arguments* args);
  void ReleaseSingleInstanceLock();
  bool Relaunch(gin::Arguments* args);
  void DisableHardwareAcceleration(gin_helper::ErrorThrower thrower);
  void DisableDomainBlockingFor3DAPIs(gin_helper::ErrorThrower thrower);
  bool IsAccessibilitySupportEnabled();
  void SetAccessibilitySupportEnabled(gin_helper::ErrorThrower thrower,
                                      bool enabled);
  Browser::LoginItemSettings GetLoginItemSettings(gin::Arguments* args);
#if defined(USE_NSS_CERTS)
  void ImportCertificate(gin_helper::ErrorThrower thrower,
                         base::Value options,
                         net::CompletionOnceCallback callback);
#endif
  v8::Local<v8::Promise> GetFileIcon(const base::FilePath& path,
                                     gin::Arguments* args);

  std::vector<gin_helper::Dictionary> GetAppMetrics(v8::Isolate* isolate);
  v8::Local<v8::Value> GetGPUFeatureStatus(v8::Isolate* isolate);
  v8::Local<v8::Promise> GetGPUInfo(v8::Isolate* isolate,
                                    const std::string& info_type);
  void EnableSandbox(gin_helper::ErrorThrower thrower);
  void SetUserAgentFallback(const std::string& user_agent);
  std::string GetUserAgentFallback();

#if defined(OS_MAC)
  void SetActivationPolicy(gin_helper::ErrorThrower thrower,
                           const std::string& policy);
  bool MoveToApplicationsFolder(gin_helper::ErrorThrower, gin::Arguments* args);
  bool IsInApplicationsFolder();
  v8::Local<v8::Value> GetDockAPI(v8::Isolate* isolate);
  bool IsRunningUnderRosettaTranslation() const;
  v8::Global<v8::Value> dock_;
#endif

#if defined(OS_MAC) || defined(OS_WIN)
  bool IsRunningUnderARM64Translation() const;
#endif

#if defined(MAS_BUILD)
  base::RepeatingCallback<void()> StartAccessingSecurityScopedResource(
      gin::Arguments* args);
#endif

#if defined(OS_WIN)
  // 获取当前的跳转列表设置。
  v8::Local<v8::Value> GetJumpListSettings();

  // 设置或删除应用程序的自定义跳转列表。
  JumpListResult SetJumpList(v8::Local<v8::Value> val, gin::Arguments* args);
#endif  // 已定义(OS_WIN)。

  std::unique_ptr<ProcessSingleton> process_singleton_;

#if defined(USE_NSS_CERTS)
  std::unique_ptr<CertificateManagerModel> certificate_manager_model_;
#endif

  // 跟踪请求文件图标的任务。
  base::CancelableTaskTracker cancelable_task_tracker_;

  base::FilePath app_path_;

  using ProcessMetricMap =
      std::map<int, std::unique_ptr<electron::ProcessMetric>>;
  ProcessMetricMap app_metrics_;

  bool disable_hw_acceleration_ = false;
  bool disable_domain_blocking_for_3DAPIs_ = false;

  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Electronics_API_APP_H_
