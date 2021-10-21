// 版权所有(C)2012 Chromium作者。版权所有。
// 版权所有(C)2013 Adam Roben&lt;adam@roben.org&gt;。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_COMMON_MAC_MAIN_APPLICATION_BUNDLE_H_
#define SHELL_COMMON_MAC_MAIN_APPLICATION_BUNDLE_H_

#ifdef __OBJC__
@class NSBundle;
#else
struct NSBundle;
#endif

namespace base {
class FilePath;
}

namespace electron {

// “主”应用程序捆绑包是此逻辑的最外层捆绑包。
// 申请。例如，如果您有MyApp.app和。
// MyApp.app/Contents/Frameworks/MyApp Helper.app，主应用程序包。
// 是MyApp.app，无论当前运行的是哪个可执行文件。
NSBundle* MainApplicationBundle();
base::FilePath MainApplicationBundlePath();

}  // 命名空间电子。

#endif  // Shell_COMMON_MAC_Main_APPLICATION_BRAND_H_
