// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/certificate_trust.h"

#include <windows.h>  // 必须先包含windows.h。

#include <wincrypt.h>

#include "net/cert/cert_database.h"
#include "net/cert/x509_util_win.h"
#include "shell/browser/javascript_environment.h"

namespace certificate_trust {

// 将提供的证书添加到受信任的根证书颁发机构。
// 用于当前用户的存储区。
// 
// 这需要提示用户确认他们信任该证书。
BOOL AddToTrustedRootStore(const PCCERT_CONTEXT cert_context,
                           const scoped_refptr<net::X509Certificate>& cert) {
  auto* root_cert_store = CertOpenStore(
      CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_SYSTEM_STORE_CURRENT_USER, L"Root");

  if (root_cert_store == NULL) {
    return false;
  }

  auto result = CertAddCertificateContextToStore(
      root_cert_store, cert_context, CERT_STORE_ADD_REPLACE_EXISTING, NULL);

  if (result) {
    // 强制Chromium重新加载此证书的数据库。
    auto* cert_db = net::CertDatabase::GetInstance();
    cert_db->NotifyObserversCertDBChanged();
  }

  CertCloseStore(root_cert_store, CERT_CLOSE_STORE_FORCE_FLAG);

  return result;
}

CERT_CHAIN_PARA GetCertificateChainParameters() {
  CERT_ENHKEY_USAGE enhkey_usage;
  enhkey_usage.cUsageIdentifier = 0;
  enhkey_usage.rgpszUsageIdentifier = NULL;

  CERT_USAGE_MATCH cert_usage;
  // 确保将规则应用于整个链。
  cert_usage.dwType = USAGE_MATCH_TYPE_AND;
  cert_usage.Usage = enhkey_usage;

  CERT_CHAIN_PARA params = {sizeof(CERT_CHAIN_PARA)};
  params.RequestedUsage = cert_usage;

  return params;
}

v8::Local<v8::Promise> ShowCertificateTrust(
    electron::NativeWindow* parent_window,
    const scoped_refptr<net::X509Certificate>& cert,
    const std::string& message) {
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  PCCERT_CHAIN_CONTEXT chain_context;
  auto cert_context = net::x509_util::CreateCertContextWithChain(cert.get());
  auto params = GetCertificateChainParameters();

  if (CertGetCertificateChain(NULL, cert_context.get(), NULL, NULL, &params,
                              NULL, NULL, &chain_context)) {
    auto error_status = chain_context->TrustStatus.dwErrorStatus;
    if (error_status == CERT_TRUST_IS_SELF_SIGNED ||
        error_status == CERT_TRUST_IS_UNTRUSTED_ROOT) {
      // 这些是我们唯一有兴趣支持的场景。
      AddToTrustedRootStore(cert_context.get(), cert);
    }

    CertFreeCertificateChain(chain_context);
  }

  promise.Resolve();
  return handle;
}

}  // 命名空间证书信任(_TRUST)
