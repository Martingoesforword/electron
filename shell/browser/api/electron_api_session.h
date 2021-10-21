// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_SESSION_H_
#define SHELL_BROWSER_API_ELECTRON_API_SESSION_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "content/public/browser/download_manager.h"
#include "electron/buildflags/buildflags.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "services/network/public/mojom/ssl_config.mojom.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/net/resolve_proxy_helper.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/function_template_extensions.h"
#include "shell/common/gin_helper/pinnable.h"
#include "shell/common/gin_helper/promise.h"

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "chrome/browser/spellchecker/spellcheck_hunspell_dictionary.h"  // 点名检查。
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_observer.h"
#endif

class GURL;

namespace base {
class FilePath;
}

namespace gin_helper {
class Dictionary;
}

namespace gin {
class Arguments;
}

namespace net {
class ProxyConfig;
}

namespace electron {

class ElectronBrowserContext;

namespace api {

class Session : public gin::Wrappable<Session>,
                public gin_helper::Pinnable<Session>,
                public gin_helper::EventEmitterMixin<Session>,
                public gin_helper::CleanedUpAtExit,
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
                public SpellcheckHunspellDictionary::Observer,
#endif
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
                public extensions::ExtensionRegistryObserver,
#endif
                public content::DownloadManager::Observer {
 public:
  // 从|BROWSER_CONTEXT|获取或创建会话。
  static gin::Handle<Session> CreateFrom(
      v8::Isolate* isolate,
      ElectronBrowserContext* browser_context);

  static Session* FromBrowserContext(content::BrowserContext* context);

  // 获取|Partition|的会话。
  static gin::Handle<Session> FromPartition(
      v8::Isolate* isolate,
      const std::string& partition,
      base::DictionaryValue options = base::DictionaryValue());

  ElectronBrowserContext* browser_context() const { return browser_context_; }

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // 方法：研究方法。
  v8::Local<v8::Promise> ResolveProxy(gin::Arguments* args);
  v8::Local<v8::Promise> GetCacheSize();
  v8::Local<v8::Promise> ClearCache();
  v8::Local<v8::Promise> ClearStorageData(gin::Arguments* args);
  void FlushStorageData();
  v8::Local<v8::Promise> SetProxy(gin::Arguments* args);
  v8::Local<v8::Promise> ForceReloadProxyConfig();
  void SetDownloadPath(const base::FilePath& path);
  void EnableNetworkEmulation(const gin_helper::Dictionary& options);
  void DisableNetworkEmulation();
  void SetCertVerifyProc(v8::Local<v8::Value> proc, gin::Arguments* args);
  void SetPermissionRequestHandler(v8::Local<v8::Value> val,
                                   gin::Arguments* args);
  void SetPermissionCheckHandler(v8::Local<v8::Value> val,
                                 gin::Arguments* args);
  void SetDevicePermissionHandler(v8::Local<v8::Value> val,
                                  gin::Arguments* args);
  v8::Local<v8::Promise> ClearHostResolverCache(gin::Arguments* args);
  v8::Local<v8::Promise> ClearAuthCache();
  void AllowNTLMCredentialsForDomains(const std::string& domains);
  void SetUserAgent(const std::string& user_agent, gin::Arguments* args);
  std::string GetUserAgent();
  void SetSSLConfig(network::mojom::SSLConfigPtr config);
  bool IsPersistent();
  v8::Local<v8::Promise> GetBlobData(v8::Isolate* isolate,
                                     const std::string& uuid);
  void DownloadURL(const GURL& url);
  void CreateInterruptedDownload(const gin_helper::Dictionary& options);
  void SetPreloads(const std::vector<base::FilePath>& preloads);
  std::vector<base::FilePath> GetPreloads() const;
  v8::Local<v8::Value> Cookies(v8::Isolate* isolate);
  v8::Local<v8::Value> Protocol(v8::Isolate* isolate);
  v8::Local<v8::Value> ServiceWorkerContext(v8::Isolate* isolate);
  v8::Local<v8::Value> WebRequest(v8::Isolate* isolate);
  v8::Local<v8::Value> NetLog(v8::Isolate* isolate);
  void Preconnect(const gin_helper::Dictionary& options, gin::Arguments* args);
  v8::Local<v8::Promise> CloseAllConnections();
  v8::Local<v8::Value> GetPath(v8::Isolate* isolate);
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  base::Value GetSpellCheckerLanguages();
  void SetSpellCheckerLanguages(gin_helper::ErrorThrower thrower,
                                const std::vector<std::string>& languages);
  v8::Local<v8::Promise> ListWordsInSpellCheckerDictionary();
  bool AddWordToSpellCheckerDictionary(const std::string& word);
  bool RemoveWordFromSpellCheckerDictionary(const std::string& word);
  void SetSpellCheckerEnabled(bool b);
  bool IsSpellCheckerEnabled() const;
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  v8::Local<v8::Promise> LoadExtension(const base::FilePath& extension_path,
                                       gin::Arguments* args);
  void RemoveExtension(const std::string& extension_id);
  v8::Local<v8::Value> GetExtension(const std::string& extension_id);
  v8::Local<v8::Value> GetAllExtensions();

  // Extensions：：ExtensionRegistryWatch：
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;
  void OnExtensionReady(content::BrowserContext* browser_context,
                        const extensions::Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionReason reason) override;
#endif

 protected:
  Session(v8::Isolate* isolate, ElectronBrowserContext* browser_context);
  ~Session() override;

  // 内容：：下载管理器：：观察者：
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  // Spellcheck HunspellDictionary：：观察者。
  void OnHunspellDictionaryInitialized(const std::string& language) override;
  void OnHunspellDictionaryDownloadBegin(const std::string& language) override;
  void OnHunspellDictionaryDownloadSuccess(
      const std::string& language) override;
  void OnHunspellDictionaryDownloadFailure(
      const std::string& language) override;
#endif

 private:
  // 缓存的gin_helper：：Wrappable对象。
  v8::Global<v8::Value> cookies_;
  v8::Global<v8::Value> protocol_;
  v8::Global<v8::Value> net_log_;
  v8::Global<v8::Value> service_worker_context_;
  v8::Global<v8::Value> web_request_;

  v8::Isolate* isolate_;

  // 启用网络限制器的客户端ID。
  base::UnguessableToken network_emulation_token_;

  ElectronBrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(Session);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Electronics_API_Session_H_
