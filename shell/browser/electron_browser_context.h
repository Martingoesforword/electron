// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_BROWSER_CONTEXT_H_
#define SHELL_BROWSER_ELECTRON_BROWSER_CONTEXT_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/predictors/preconnect_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/resource_context.h"
#include "electron/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "shell/browser/media/media_device_id_salt.h"

class PrefService;
class ValueMapPrefStore;

namespace network {
class SharedURLLoaderFactory;
}

namespace storage {
class SpecialStoragePolicy;
}

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
namespace extensions {
class ElectronExtensionSystem;
}
#endif

namespace electron {

class ElectronBrowserContext;
class ElectronDownloadManagerDelegate;
class ElectronPermissionManager;
class CookieChangeNotifier;
class ResolveProxyHelper;
class WebViewManager;
class ProtocolRegistry;

class ElectronBrowserContext : public content::BrowserContext {
 public:
  // 分区id=&gt;浏览器上下文。
  struct PartitionKey {
    std::string partition;
    bool in_memory;

    PartitionKey(const std::string& partition, bool in_memory)
        : partition(partition), in_memory(in_memory) {}

    bool operator<(const PartitionKey& other) const {
      if (partition == other.partition)
        return in_memory < other.in_memory;
      return partition < other.partition;
    }

    bool operator==(const PartitionKey& other) const {
      return (partition == other.partition) && (in_memory == other.in_memory);
    }
  };
  using BrowserContextMap =
      std::map<PartitionKey, std::unique_ptr<ElectronBrowserContext>>;

  // 根据BrowserContext的|分区|获取或创建BrowserContext。
  // In_memory|。如果没有|Options|，则会将|Options|传递给构造函数。
  // 现有的BrowserContext。
  static ElectronBrowserContext* From(
      const std::string& partition,
      bool in_memory,
      base::DictionaryValue options = base::DictionaryValue());

  static BrowserContextMap& browser_context_map();

  void SetUserAgent(const std::string& user_agent);
  std::string GetUserAgent() const;
  bool CanUseHttpCache() const;
  int GetMaxCacheSize() const;
  ResolveProxyHelper* GetResolveProxyHelper();
  predictors::PreconnectManager* GetPreconnectManager();
  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory();

  // 内容：：BrowserContext：
  base::FilePath GetPath() override;
  bool IsOffTheRecord() override;
  content::ResourceContext* GetResourceContext() override;
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate()
      override;
  std::string GetMediaDeviceIDSalt() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  content::PlatformNotificationService* GetPlatformNotificationService()
      override;
  content::PermissionControllerDelegate* GetPermissionControllerDelegate()
      override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::ClientHintsControllerDelegate* GetClientHintsControllerDelegate()
      override;
  content::StorageNotificationService* GetStorageNotificationService() override;

  CookieChangeNotifier* cookie_change_notifier() const {
    return cookie_change_notifier_.get();
  }
  PrefService* prefs() const { return prefs_.get(); }
  void set_in_memory_pref_store(ValueMapPrefStore* pref_store) {
    in_memory_pref_store_ = pref_store;
  }
  ValueMapPrefStore* in_memory_pref_store() const {
    return in_memory_pref_store_;
  }
  base::WeakPtr<ElectronBrowserContext> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions::ElectronExtensionSystem* extension_system() {
    // 使用！IsOffTheRecord()保护Extension_System()的用法。
    // 没有用于内存中会话的扩展系统。
    DCHECK(!IsOffTheRecord());
    return extension_system_;
  }
#endif

  ProtocolRegistry* protocol_registry() const {
    return protocol_registry_.get();
  }

  void SetSSLConfig(network::mojom::SSLConfigPtr config);
  network::mojom::SSLConfigPtr GetSSLConfig();
  void SetSSLConfigClient(mojo::Remote<network::mojom::SSLConfigClient> client);

  ~ElectronBrowserContext() override;

 private:
  ElectronBrowserContext(const std::string& partition,
                         bool in_memory,
                         base::DictionaryValue options);

  // 初始化首选项注册表。
  void InitPrefs();

  ValueMapPrefStore* in_memory_pref_store_ = nullptr;

  std::unique_ptr<content::ResourceContext> resource_context_;
  std::unique_ptr<CookieChangeNotifier> cookie_change_notifier_;
  std::unique_ptr<PrefService> prefs_;
  std::unique_ptr<ElectronDownloadManagerDelegate> download_manager_delegate_;
  std::unique_ptr<WebViewManager> guest_manager_;
  std::unique_ptr<ElectronPermissionManager> permission_manager_;
  std::unique_ptr<MediaDeviceIDSalt> media_device_id_salt_;
  scoped_refptr<ResolveProxyHelper> resolve_proxy_helper_;
  scoped_refptr<storage::SpecialStoragePolicy> storage_policy_;
  std::unique_ptr<predictors::PreconnectManager> preconnect_manager_;
  std::unique_ptr<ProtocolRegistry> protocol_registry_;

  std::string user_agent_;
  base::FilePath path_;
  bool in_memory_ = false;
  bool use_cache_ = true;
  int max_cache_size_ = 0;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // 由KeyedService系统拥有。
  extensions::ElectronExtensionSystem* extension_system_;
#endif

  // 共享URLLoaderFactory。
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  network::mojom::SSLConfigPtr ssl_config_;
  mojo::Remote<network::mojom::SSLConfigClient> ssl_config_client_;

  base::WeakPtrFactory<ElectronBrowserContext> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ElectronBrowserContext);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Electronics_Browser_Context_H_
