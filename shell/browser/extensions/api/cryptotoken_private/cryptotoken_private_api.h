// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_API_CRYPTOTOKEN_PRIVATE_CRYPTOTOKEN_PRIVATE_API_H_
#define SHELL_BROWSER_EXTENSIONS_API_CRYPTOTOKEN_PRIVATE_CRYPTOTOKEN_PRIVATE_API_H_

#include "chrome/common/extensions/api/cryptotoken_private.h"
#include "extensions/browser/extension_function.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

// Chrome.cryptokenPrivate API函数的实现。

namespace extensions {
namespace api {

// Void CryptokenRegisterProfilePrefs(。
// User_prefs：：PrefRegistrySynCable*注册表)；

class CryptotokenPrivateCanOriginAssertAppIdFunction
    : public ExtensionFunction {
 public:
  CryptotokenPrivateCanOriginAssertAppIdFunction();
  DECLARE_EXTENSION_FUNCTION("cryptotokenPrivate.canOriginAssertAppId",
                             CRYPTOTOKENPRIVATE_CANORIGINASSERTAPPID)
 protected:
  ~CryptotokenPrivateCanOriginAssertAppIdFunction() override {}
  ResponseAction Run() override;
};

class CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction
    : public ExtensionFunction {
 public:
  CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction();
  DECLARE_EXTENSION_FUNCTION(
      "cryptotokenPrivate.isAppIdHashInEnterpriseContext",
      CRYPTOTOKENPRIVATE_ISAPPIDHASHINENTERPRISECONTEXT)

 protected:
  ~CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction() override {}
  ResponseAction Run() override;
};

class CryptotokenPrivateCanAppIdGetAttestationFunction
    : public ExtensionFunction {
 public:
  CryptotokenPrivateCanAppIdGetAttestationFunction();
  DECLARE_EXTENSION_FUNCTION("cryptotokenPrivate.canAppIdGetAttestation",
                             CRYPTOTOKENPRIVATE_CANAPPIDGETATTESTATION)

 protected:
  ~CryptotokenPrivateCanAppIdGetAttestationFunction() override {}
  ResponseAction Run() override;
  void Complete(bool result);
};

class CryptotokenPrivateRecordRegisterRequestFunction
    : public ExtensionFunction {
 public:
  CryptotokenPrivateRecordRegisterRequestFunction() = default;
  DECLARE_EXTENSION_FUNCTION("cryptotokenPrivate.recordRegisterRequest",
                             CRYPTOTOKENPRIVATE_RECORDREGISTERREQUEST)

 protected:
  ~CryptotokenPrivateRecordRegisterRequestFunction() override = default;
  ResponseAction Run() override;
};

class CryptotokenPrivateRecordSignRequestFunction : public ExtensionFunction {
 public:
  CryptotokenPrivateRecordSignRequestFunction() = default;
  DECLARE_EXTENSION_FUNCTION("cryptotokenPrivate.recordSignRequest",
                             CRYPTOTOKENPRIVATE_RECORDSIGNREQUEST)

 protected:
  ~CryptotokenPrivateRecordSignRequestFunction() override = default;
  ResponseAction Run() override;
};

}  // 命名空间API。
}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_API_CRYPTOTOKEN_PRIVATE_CRYPTOTOKEN_PRIVATE_API_H_
