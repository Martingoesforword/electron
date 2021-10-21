// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_ELECTRON_CONSTANTS_H_
#define SHELL_COMMON_ELECTRON_CONSTANTS_H_

#include "base/files/file_path.h"
#include "build/build_config.h"
#include "electron/buildflags/buildflags.h"

namespace electron {

// NativeWindow中的app命令。
extern const char kBrowserForward[];
extern const char kBrowserBackward[];

// 描述DevTools安全面板的Chrome安全策略的字符串。
extern const char kSHA1Certificate[];
extern const char kSHA1MajorDescription[];
extern const char kSHA1MinorDescription[];
extern const char kCertificateError[];
extern const char kValidCertificate[];
extern const char kValidCertificateDescription[];
extern const char kSecureProtocol[];
extern const char kSecureProtocolDescription[];

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
extern const char kRunAsNode[];
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
// 用于PDF插件的MIME类型。
extern const char kPdfPluginMimeType[];
extern const base::FilePath::CharType kPdfPluginPath[];
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)。

}  // 命名空间电子。

#endif  // 壳层公共电子常数H_
