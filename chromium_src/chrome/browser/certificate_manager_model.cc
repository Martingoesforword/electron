// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "chrome/browser/certificate_manager_model.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_context.h"
#include "crypto/nss_util.h"
#include "crypto/nss_util_internal.h"
#include "net/base/net_errors.h"
#include "net/cert/nss_cert_database.h"
#include "net/cert/x509_certificate.h"

using content::BrowserThread;

namespace {

net::NSSCertDatabase* g_nss_cert_database = nullptr;

net::NSSCertDatabase* GetNSSCertDatabaseForResourceContext(
    content::ResourceContext* context,
    base::OnceCallback<void(net::NSSCertDatabase*)> callback) {
  // 此初始化不是线程安全的。此检查确保此代码。
  // 仅在单个线程上运行。
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  if (!g_nss_cert_database) {
    // 与ChromeOS的独立插槽相比，Linux只有一个持久插槽。
    // 公共和私人插槽。
    // 将所有插槽使用情况重定向到Linux上的此永久插槽。
    crypto::EnsureNSSInit();
    g_nss_cert_database = new net::NSSCertDatabase(
        crypto::ScopedPK11Slot(PK11_GetInternalKeySlot()) /* 公共插槽。*/,
        crypto::ScopedPK11Slot(PK11_GetInternalKeySlot()) /* 专用插槽。*/);
  }
  return g_nss_cert_database;
}

}  // 命名空间。

// CertificateManagerModel是在UI线程上创建的。它需要一个。
// NSSCertDatabase句柄(在ChromeOS上，它需要获取TPM状态)。
// 需要在IO线程上完成。
// 
// 初始化流程大致为：
// 
// UI线程IO线程。
// 
// CertificateManagerModel：：Create。
// \。
// CertificateManagerModel：：GetCertDBOnIOThread。
// |。
// GetNSSCertDatabaseForResourceContext。
// |。
// CertificateManagerModel：：DidGetCertDBOnIOThread。
// V-/。
// CertificateManagerModel：：DidGetCertDBOnUIThread。
// |。
// 新的CertificateManagerModel。
// |。
// 回调。

// 静电。
void CertificateManagerModel::Create(content::BrowserContext* browser_context,
                                     CreationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  base::PostTask(FROM_HERE, {BrowserThread::IO},
                 base::BindOnce(&CertificateManagerModel::GetCertDBOnIOThread,
                                browser_context->GetResourceContext(),
                                std::move(callback)));
}

CertificateManagerModel::CertificateManagerModel(
    net::NSSCertDatabase* nss_cert_database,
    bool is_user_db_available)
    : cert_db_(nss_cert_database), is_user_db_available_(is_user_db_available) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

CertificateManagerModel::~CertificateManagerModel() = default;

int CertificateManagerModel::ImportFromPKCS12(
    PK11SlotInfo* slot_info,
    const std::string& data,
    const std::u16string& password,
    bool is_extractable,
    net::ScopedCERTCertificateList* imported_certs) {
  return cert_db_->ImportFromPKCS12(slot_info, data, password, is_extractable,
                                    imported_certs);
}

int CertificateManagerModel::ImportUserCert(const std::string& data) {
  return cert_db_->ImportUserCert(data);
}

bool CertificateManagerModel::ImportCACerts(
    const net::ScopedCERTCertificateList& certificates,
    net::NSSCertDatabase::TrustBits trust_bits,
    net::NSSCertDatabase::ImportCertFailureList* not_imported) {
  return cert_db_->ImportCACerts(certificates, trust_bits, not_imported);
}

bool CertificateManagerModel::ImportServerCert(
    const net::ScopedCERTCertificateList& certificates,
    net::NSSCertDatabase::TrustBits trust_bits,
    net::NSSCertDatabase::ImportCertFailureList* not_imported) {
  return cert_db_->ImportServerCert(certificates, trust_bits, not_imported);
}

bool CertificateManagerModel::SetCertTrust(
    CERTCertificate* cert,
    net::CertType type,
    net::NSSCertDatabase::TrustBits trust_bits) {
  return cert_db_->SetCertTrust(cert, type, trust_bits);
}

bool CertificateManagerModel::Delete(CERTCertificate* cert) {
  return cert_db_->DeleteCertAndKey(cert);
}

// 静电。
void CertificateManagerModel::DidGetCertDBOnUIThread(
    net::NSSCertDatabase* cert_db,
    bool is_user_db_available,
    CreationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto model = base::WrapUnique(
      new CertificateManagerModel(cert_db, is_user_db_available));
  std::move(callback).Run(std::move(model));
}

// 静电。
void CertificateManagerModel::DidGetCertDBOnIOThread(
    CreationCallback callback,
    net::NSSCertDatabase* cert_db) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  bool is_user_db_available = !!cert_db->GetPublicSlot();
  base::PostTask(
      FROM_HERE, {BrowserThread::UI},
      base::BindOnce(&CertificateManagerModel::DidGetCertDBOnUIThread, cert_db,
                     is_user_db_available, std::move(callback)));
}

// 静电。
void CertificateManagerModel::GetCertDBOnIOThread(
    content::ResourceContext* context,
    CreationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  auto split_callback = base::SplitOnceCallback(base::BindOnce(
      &CertificateManagerModel::DidGetCertDBOnIOThread, std::move(callback)));

  net::NSSCertDatabase* cert_db = GetNSSCertDatabaseForResourceContext(
      context, std::move(split_callback.first));

  // 如果NSS数据库已可用，|cert_db|为非空，
  // |DID_GET_CERT_db_CALLBACK|未调用。明确地说。
  if (cert_db)
    std::move(split_callback.second).Run(cert_db);
}
