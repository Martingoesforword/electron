// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_RENDERER_CLIENT_BASE_H_
#define SHELL_RENDERER_RENDERER_CLIENT_BASE_H_

#include <memory>
#include <string>
#include <vector>

#include "content/public/renderer/content_renderer_client.h"
#include "electron/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "shell/common/gin_helper/dictionary.h"
// 在SHARED_MIDENTAL_DIR中。
#include "widevine_cdm_version.h"  // NOLINT(BUILD/INCLUDE_DIRECTORY)。

#if defined(WIDEVINE_CDM_AVAILABLE)
#include "chrome/renderer/media/chrome_key_systems_provider.h"  // 点名检查。
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "chrome/renderer/pepper/chrome_pdf_print_client.h"  // 点名检查。
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)。

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/local_interface_provider.h"

class SpellCheck;
#endif

namespace blink {
class WebLocalFrame;
}

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
namespace extensions {
class ExtensionsClient;
}
#endif

namespace electron {

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
class ElectronExtensionsRendererClient;
#endif

class RendererClientBase : public content::ContentRendererClient
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
    ,
                           public service_manager::LocalInterfaceProvider
#endif
{
 public:
  RendererClientBase();
  ~RendererClientBase() override;

  static RendererClientBase* Get();

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  // SERVICE_MANAGER：：LocalInterfaceProvider实现。
  void GetInterface(const std::string& name,
                    mojo::ScopedMessagePipeHandle interface_pipe) override;
#endif

  virtual void DidCreateScriptContext(v8::Handle<v8::Context> context,
                                      content::RenderFrame* render_frame) = 0;
  virtual void WillReleaseScriptContext(v8::Handle<v8::Context> context,
                                        content::RenderFrame* render_frame) = 0;
  virtual void DidClearWindowObject(content::RenderFrame* render_frame);
  virtual void SetupMainWorldOverrides(v8::Handle<v8::Context> context,
                                       content::RenderFrame* render_frame);

  std::unique_ptr<blink::WebPrescientNetworking> CreatePrescientNetworking(
      content::RenderFrame* render_frame) override;

  // 获取运行Electron API的上下文。
  v8::Local<v8::Context> GetContext(blink::WebLocalFrame* frame,
                                    v8::Isolate* isolate) const;

  static void AllowGuestViewElementDefinition(
      v8::Isolate* isolate,
      v8::Local<v8::Object> context,
      v8::Local<v8::Function> register_cb);

  bool IsWebViewFrame(v8::Handle<v8::Context> context,
                      content::RenderFrame* render_frame) const;

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  SpellCheck* GetSpellCheck() { return spellcheck_.get(); }
#endif

 protected:
  void BindProcess(v8::Isolate* isolate,
                   gin_helper::Dictionary* process,
                   content::RenderFrame* render_frame);

  // 内容：：ContentRendererClient：
  void RenderThreadStarted() override;
  void ExposeInterfacesToBrowser(mojo::BinderMap* binders) override;
  void RenderFrameCreated(content::RenderFrame*) override;
  bool OverrideCreatePlugin(content::RenderFrame* render_frame,
                            const blink::WebPluginParams& params,
                            blink::WebPlugin** plugin) override;
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems)
      override;
  bool IsKeySystemsUpdateNeeded() override;
  void DidSetUserAgent(const std::string& user_agent) override;
  bool IsPluginHandledExternally(content::RenderFrame* render_frame,
                                 const blink::WebElement& plugin_element,
                                 const GURL& original_url,
                                 const std::string& mime_type) override;
  bool IsOriginIsolatedPepperPlugin(const base::FilePath& plugin_path) override;

  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentIdle(content::RenderFrame* render_frame) override;

  bool AllowScriptExtensionForServiceWorker(
      const url::Origin& script_origin) override;
  void DidInitializeServiceWorkerContextOnWorkerThread(
      blink::WebServiceWorkerContextProxy* context_proxy,
      const GURL& service_worker_scope,
      const GURL& script_url) override;
  void WillEvaluateServiceWorkerOnWorkerThread(
      blink::WebServiceWorkerContextProxy* context_proxy,
      v8::Local<v8::Context> v8_context,
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url) override;
  void DidStartServiceWorkerContextOnWorkerThread(
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url) override;
  void WillDestroyServiceWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context,
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url) override;

 protected:
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // APP_SHELL嵌入器可能需要自定义扩展客户端接口。
  // 此类取得返回对象的所有权。
  virtual extensions::ExtensionsClient* CreateExtensionsClient();
#endif

 private:
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  std::unique_ptr<extensions::ExtensionsClient> extensions_client_;
  std::unique_ptr<ElectronExtensionsRendererClient> extensions_renderer_client_;
#endif

#if defined(WIDEVINE_CDM_AVAILABLE)
  ChromeKeySystemsProvider key_systems_provider_;
#endif
  std::string renderer_client_id_;
  // 用于标识此流程中的V8上下文的递增ID。
  int64_t next_context_id_ = 0;

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  std::unique_ptr<SpellCheck> spellcheck_;
#endif
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  std::unique_ptr<ChromePDFPrintClient> pdf_print_client_;
#endif
};

}  // 命名空间电子。

#endif  // SHELL_RENDER_RENDER_CLIENT_BASE_H_
