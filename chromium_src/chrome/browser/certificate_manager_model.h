// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef CHROME_BROWSER_CERTIFICATE_MANAGER_MODEL_H_
#define CHROME_BROWSER_CERTIFICATE_MANAGER_MODEL_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/cert/nss_cert_database.h"

namespace content {
class BrowserContext;
class ResourceContext;
}  // 命名空间内容。

// CertificateManagerModel提供要在证书中显示的数据。
// “管理器”对话框，并处理视图中的更改。
class CertificateManagerModel {
 public:
  using CreationCallback =
      base::OnceCallback<void(std::unique_ptr<CertificateManagerModel>)>;

  // 创建一个CertificateManagerModel。该模型将被传递给回调。
  // 当它准备好的时候。调用者必须确保模型的寿命不会超过。
  // |BROWSER_CONTEXT|。
  static void Create(content::BrowserContext* browser_context,
                     CreationCallback callback);

  ~CertificateManagerModel();

  bool is_user_db_available() const { return is_user_db_available_; }

  // 访问者对基础NSSCertDatabase进行只读访问。
  const net::NSSCertDatabase* cert_db() const { return cert_db_; }

  // 从编码的PKCS#12导入私钥和证书。
  // Data|，使用给定的|密码。如果|IS_EXTRACTABLE|为FALSE，
  // 将私钥标记为无法从模块中提取。
  // 失败时返回网络错误代码。
  int ImportFromPKCS12(PK11SlotInfo* slot_info,
                       const std::string& data,
                       const std::u16string& password,
                       bool is_extractable,
                       net::ScopedCERTCertificateList* imported_certs);

  // 从DER编码|数据|导入用户证书。
  // 失败时返回网络错误代码。
  int ImportUserCert(const std::string& data);

  // 导入CA证书。
  // 尝试导入所有给定的证书。根目录将是受信任的。
  // 根据|TRUST_BITS|。任何无法导入的证书。
  // 将列在|NOT_IMPORTED|中。
  // |TRUST_BITS|应该是来自NSSCertDatabase的信任*值的位字段。
  // 如果存在内部错误，则返回FALSE，否则返回TRUE，
  // |NOT_IMPORTED|应检查是否有未导入的证书。
  // 进口的。
  bool ImportCACerts(const net::ScopedCERTCertificateList& certificates,
                     net::NSSCertDatabase::TrustBits trust_bits,
                     net::NSSCertDatabase::ImportCertFailureList* not_imported);

  // 导入服务器证书。第一个证书应该是服务器证书。任何。
  // 其他证书应该是中级/CA证书，并且将被导入，但是。
  // 没有得到任何信任。
  // 任何无法导入的证书都将列在。
  // NOT_IMPORTED|。
  // |TRUST_BITS|可以设置为显式信任或不信任证书，或者。
  // 使用TRUST_DEFAULT照常继承信任。
  // 如果存在内部错误，则返回FALSE，否则返回TRUE，
  // |NOT_IMPORTED|应检查是否有未导入的证书。
  // 进口的。
  bool ImportServerCert(
      const net::ScopedCERTCertificateList& certificates,
      net::NSSCertDatabase::TrustBits trust_bits,
      net::NSSCertDatabase::ImportCertFailureList* not_imported);

  // 设置证书的信任值。
  // |TRUST_BITS|应该是来自NSSCertDatabase的信任*值的位字段。
  // 成功时返回TRUE，失败时返回FALSE。
  bool SetCertTrust(CERTCertificate* cert,
                    net::CertType type,
                    net::NSSCertDatabase::TrustBits trust_bits);

  // 删除证书。如果成功，则返回TRUE。|cert|在此情况下仍然有效。
  // 函数返回。
  bool Delete(CERTCertificate* cert);

 private:
  CertificateManagerModel(net::NSSCertDatabase* nss_cert_database,
                          bool is_user_db_available);

  // 方法，请参见.cc文件顶部的注释。
  // 有关详细信息，请提交文件。
  static void DidGetCertDBOnUIThread(net::NSSCertDatabase* cert_db,
                                     bool is_user_db_available,
                                     CreationCallback callback);
  static void DidGetCertDBOnIOThread(CreationCallback callback,
                                     net::NSSCertDatabase* cert_db);
  static void GetCertDBOnIOThread(content::ResourceContext* context,
                                  CreationCallback callback);

  net::NSSCertDatabase* cert_db_;
  // 证书数据库是否具有与。
  // 侧写。如果未设置，则此型号不允许导入证书。
  bool is_user_db_available_;

  DISALLOW_COPY_AND_ASSIGN(CertificateManagerModel);
};

#endif  // Chrome_Browser_Certificate_Manager_Model_H_
