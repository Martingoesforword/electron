// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_CERTIFICATE_TRUST_H_
#define SHELL_BROWSER_UI_CERTIFICATE_TRUST_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "net/cert/x509_certificate.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/promise.h"

namespace electron {
class NativeWindow;
}

namespace certificate_trust {

v8::Local<v8::Promise> ShowCertificateTrust(
    electron::NativeWindow* parent_window,
    const scoped_refptr<net::X509Certificate>& cert,
    const std::string& message);

}  // 命名空间证书信任(_TRUST)。

#endif  // Shell_Browser_UI_CERTIFICATE_TRUST_H_
