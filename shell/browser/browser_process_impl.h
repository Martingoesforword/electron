// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

// 该接口用于管理应用程序的全局服务。每个。
// 服务是在第一次请求时延迟创建的。服务获取程序。
// 如果服务不可用，则返回NULL，因此调用方必须检查。
// 这种情况。

#ifndef SHELL_BROWSER_BROWSER_PROCESS_IMPL_H_
#define SHELL_BROWSER_BROWSER_PROCESS_IMPL_H_

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/macros.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/value_map_pref_store.h"
#include "printing/buildflags/buildflags.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "shell/browser/net/system_network_context_manager.h"

namespace printing {
class PrintJobManager;
}

// STD：：UNIQUE_PTR的定义为空，而不是转发声明。
class BackgroundModeManager {};

// 不是线程安全的，只能从主线程调用。
// 除非另有说明，否则这些函数不应返回NULL。
class BrowserProcessImpl : public BrowserProcess {
 public:
  BrowserProcessImpl();
  ~BrowserProcessImpl() override;

  static void ApplyProxyModeFromCommandLine(ValueMapPrefStore* pref_store);

  BuildState* GetBuildState() override;
  breadcrumbs::BreadcrumbPersistentStorageManager*
  GetBreadcrumbPersistentStorageManager() override;
  void PostEarlyInitialization();
  void PreCreateThreads();
  void PostDestroyThreads() {}
  void PostMainMessageLoopRun();

  void EndSession() override {}
  void FlushLocalStateAndReply(base::OnceClosure reply) override {}
  bool IsShuttingDown() override;

  metrics_services_manager::MetricsServicesManager* GetMetricsServicesManager()
      override;
  metrics::MetricsService* metrics_service() override;
  ProfileManager* profile_manager() override;
  PrefService* local_state() override;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory()
      override;
  variations::VariationsService* variations_service() override;
  BrowserProcessPlatformPart* platform_part() override;
  extensions::EventRouterForwarder* extension_event_router_forwarder() override;
  NotificationUIManager* notification_ui_manager() override;
  NotificationPlatformBridge* notification_platform_bridge() override;
  SystemNetworkContextManager* system_network_context_manager() override;
  network::NetworkQualityTracker* network_quality_tracker() override;
  WatchDogThread* watchdog_thread() override;
  policy::ChromeBrowserPolicyConnector* browser_policy_connector() override;
  policy::PolicyService* policy_service() override;
  IconManager* icon_manager() override;
  GpuModeManager* gpu_mode_manager() override;
  printing::PrintPreviewDialogController* print_preview_dialog_controller()
      override;
  printing::BackgroundPrintingManager* background_printing_manager() override;
  IntranetRedirectDetector* intranet_redirect_detector() override;
  DownloadStatusUpdater* download_status_updater() override;
  DownloadRequestLimiter* download_request_limiter() override;
  BackgroundModeManager* background_mode_manager() override;
  StatusTray* status_tray() override;
  safe_browsing::SafeBrowsingService* safe_browsing_service() override;
  subresource_filter::RulesetService* subresource_filter_ruleset_service()
      override;
  federated_learning::FlocSortingLshClustersService*
  floc_sorting_lsh_clusters_service() override;
  component_updater::ComponentUpdateService* component_updater() override;
  MediaFileSystemRegistry* media_file_system_registry() override;
  WebRtcLogUploader* webrtc_log_uploader() override;
  network_time::NetworkTimeTracker* network_time_tracker() override;
  gcm::GCMDriver* gcm_driver() override;
  resource_coordinator::ResourceCoordinatorParts* resource_coordinator_parts()
      override;
  resource_coordinator::TabManager* GetTabManager() override;
  SerialPolicyAllowedPorts* serial_policy_allowed_ports() override;
  void CreateDevToolsProtocolHandler() override {}
  void CreateDevToolsAutoOpener() override {}
  void set_background_mode_manager_for_test(
      std::unique_ptr<BackgroundModeManager> manager) override {}
#if (defined(OS_WIN) || defined(OS_LINUX))
  void StartAutoupdateTimer() override {}
#endif
  void SetApplicationLocale(const std::string& locale) override;
  const std::string& GetApplicationLocale() override;
  printing::PrintJobManager* print_job_manager() override;
  StartupData* startup_data() override;

 private:
#if BUILDFLAG(ENABLE_PRINTING)
  std::unique_ptr<printing::PrintJobManager> print_job_manager_;
#endif
  std::unique_ptr<PrefService> local_state_;
  std::string locale_;

  DISALLOW_COPY_AND_ASSIGN(BrowserProcessImpl);
};

#endif  // Shell_Browser_Browser_Process_Impll_H_
