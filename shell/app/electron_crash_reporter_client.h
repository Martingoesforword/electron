// 版权所有2013年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_APP_ELECTRON_CRASH_REPORTER_CLIENT_H_
#define SHELL_APP_ELECTRON_CRASH_REPORTER_CLIENT_H_

#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/no_destructor.h"
#include "build/build_config.h"
#include "components/crash/core/app/crash_reporter_client.h"

class ElectronCrashReporterClient : public crash_reporter::CrashReporterClient {
 public:
  static void Create();

  static ElectronCrashReporterClient* Get();
  void SetCollectStatsConsent(bool upload_allowed);
  void SetUploadUrl(const std::string& url);
  void SetShouldRateLimit(bool rate_limit);
  void SetShouldCompressUploads(bool compress_uploads);
  void SetGlobalAnnotations(
      const std::map<std::string, std::string>& annotations);

  // CRASH_REPORTER：：CrashReporterClient实现。
#if defined(OS_LINUX)
  void SetCrashReporterClientIdFromGUID(
      const std::string& client_guid) override;
  void GetProductNameAndVersion(const char** product_name,
                                const char** version) override;
  void GetProductNameAndVersion(std::string* product_name,
                                std::string* version,
                                std::string* channel) override;
  base::FilePath GetReporterLogFilename() override;
#endif

#if defined(OS_WIN)
  void GetProductNameAndVersion(const std::wstring& exe_path,
                                std::wstring* product_name,
                                std::wstring* version,
                                std::wstring* special_build,
                                std::wstring* channel_name) override;
#endif

#if defined(OS_WIN)
  bool GetCrashDumpLocation(std::wstring* crash_dir) override;
#else
  bool GetCrashDumpLocation(base::FilePath* crash_dir) override;
#endif

  bool IsRunningUnattended() override;

  bool GetCollectStatsConsent() override;

  bool GetShouldRateLimit() override;
  bool GetShouldCompressUploads() override;

  void GetProcessSimpleAnnotations(
      std::map<std::string, std::string>* annotations) override;

#if defined(OS_MAC)
  bool ReportingIsEnforcedByPolicy(bool* breakpad_enabled) override;
#endif

#if defined(OS_MAC) || defined(OS_LINUX)
  bool ShouldMonitorCrashHandlerExpensively() override;
#endif

  bool EnableBreakpadForProcess(const std::string& process_type) override;

  std::string GetUploadUrl() override;

 private:
  friend class base::NoDestructor<ElectronCrashReporterClient>;

  std::string upload_url_;
  bool collect_stats_consent_ = false;
  bool rate_limit_ = false;
  bool compress_uploads_ = false;
  std::map<std::string, std::string> global_annotations_;

  ElectronCrashReporterClient();
  ~ElectronCrashReporterClient() override;

  DISALLOW_COPY_AND_ASSIGN(ElectronCrashReporterClient);
};

#endif  // Shell_APP_ELECTIVE_CRASH_REPORTER_CLIENT_H_
