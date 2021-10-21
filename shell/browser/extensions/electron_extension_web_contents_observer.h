// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_WEB_CONTENTS_OBSERVER_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_WEB_CONTENTS_OBSERVER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/extension_web_contents_observer.h"

namespace extensions {

// ExtensionWebContentsWatch的APP_SHELL版本。
class ElectronExtensionWebContentsObserver
    : public ExtensionWebContentsObserver,
      public content::WebContentsUserData<
          ElectronExtensionWebContentsObserver> {
 public:
  ~ElectronExtensionWebContentsObserver() override;

  // 对象创建并初始化此类的实例。
  // |web_Contents|(如果不存在)。
  static void CreateForWebContents(content::WebContents* web_contents);

 private:
  friend class content::WebContentsUserData<
      ElectronExtensionWebContentsObserver>;

  explicit ElectronExtensionWebContentsObserver(
      content::WebContents* web_contents);

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionWebContentsObserver);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_WEB_CONTENTS_OBSERVER_H_
