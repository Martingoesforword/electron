// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/electron_extension_web_contents_observer.h"

namespace extensions {

ElectronExtensionWebContentsObserver::ElectronExtensionWebContentsObserver(
    content::WebContents* web_contents)
    : ExtensionWebContentsObserver(web_contents) {}

ElectronExtensionWebContentsObserver::~ElectronExtensionWebContentsObserver() =
    default;

void ElectronExtensionWebContentsObserver::CreateForWebContents(
    content::WebContents* web_contents) {
  content::WebContentsUserData<
      ElectronExtensionWebContentsObserver>::CreateForWebContents(web_contents);

  // 如有必要，初始化此实例。
  FromWebContents(web_contents)->Initialize();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ElectronExtensionWebContentsObserver)

}  // 命名空间扩展
