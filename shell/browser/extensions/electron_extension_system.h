// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_SYSTEM_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_SYSTEM_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "components/value_store/value_store_factory.h"
#include "components/value_store/value_store_factory_impl.h"
#include "extensions/browser/extension_system.h"

namespace base {
class FilePath;
}

namespace content {
class BrowserContext;
}

namespace extensions {

class ElectronExtensionLoader;
class ValueStoreFactory;

// APP_SHELL的ExtensionSystem的简化版本。允许。
// APP_SHELL跳过它不需要的服务的初始化。
class ElectronExtensionSystem : public ExtensionSystem {
 public:
  using InstallUpdateCallback = ExtensionSystem::InstallUpdateCallback;
  explicit ElectronExtensionSystem(content::BrowserContext* browser_context);
  ~ElectronExtensionSystem() override;

  // 从目录加载未打包的扩展名。返回上的分机号。
  // 成功，否则返回nullptr。
  void LoadExtension(
      const base::FilePath& extension_dir,
      int load_flags,
      base::OnceCallback<void(const Extension*, const std::string&)> cb);

  // 完成外壳扩展系统的初始化。
  void FinishInitialization();

  // 重新加载id|EXTENSION_ID|的扩展。
  void ReloadExtension(const ExtensionId& extension_id);

  void RemoveExtension(const ExtensionId& extension_id);

  // KeyedService实施：
  void Shutdown() override;

  // 扩展系统实施：
  void InitForRegularProfile(bool extensions_enabled) override;
  ExtensionService* extension_service() override;
  RuntimeData* runtime_data() override;
  ManagementPolicy* management_policy() override;
  ServiceWorkerManager* service_worker_manager() override;
  UserScriptManager* user_script_manager() override;
  StateStore* state_store() override;
  StateStore* rules_store() override;
  scoped_refptr<value_store::ValueStoreFactory> store_factory() override;
  InfoMap* info_map() override;
  QuotaService* quota_service() override;
  AppSorting* app_sorting() override;
  void RegisterExtensionWithRequestContexts(
      const Extension* extension,
      base::OnceClosure callback) override;
  void UnregisterExtensionWithRequestContexts(
      const std::string& extension_id,
      const UnloadedExtensionReason reason) override;
  const base::OneShotEvent& ready() const override;
  bool is_ready() const override;
  ContentVerifier* content_verifier() override;
  std::unique_ptr<ExtensionSet> GetDependentExtensions(
      const Extension* extension) override;
  void InstallUpdate(const std::string& extension_id,
                     const std::string& public_key,
                     const base::FilePath& temp_dir,
                     bool install_immediately,
                     InstallUpdateCallback install_update_callback) override;
  bool FinishDelayedInstallationIfReady(const std::string& extension_id,
                                        bool install_immediately) override;
  void PerformActionBasedOnOmahaAttributes(
      const std::string& extension_id,
      const base::Value& attributes) override;

 private:
  void OnExtensionRegisteredWithRequestContexts(
      scoped_refptr<Extension> extension);
  void LoadComponentExtensions();

  content::BrowserContext* browser_context_;  // 不是所有的。

  // 要在IO线程上访问的数据。必须超过PROCESS_MANAGER_。
  scoped_refptr<InfoMap> info_map_;

  std::unique_ptr<ServiceWorkerManager> service_worker_manager_;
  std::unique_ptr<RuntimeData> runtime_data_;
  std::unique_ptr<QuotaService> quota_service_;
  std::unique_ptr<UserScriptManager> user_script_manager_;
  std::unique_ptr<AppSorting> app_sorting_;
  std::unique_ptr<ManagementPolicy> management_policy_;

  std::unique_ptr<ElectronExtensionLoader> extension_loader_;

  scoped_refptr<value_store::ValueStoreFactory> store_factory_;

  // 当扩展系统完成其启动任务时发出信号。
  base::OneShotEvent ready_;

  base::WeakPtrFactory<ElectronExtensionSystem> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionSystem);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_SYSTEM_H_
