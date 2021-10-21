// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_AUTOFILL_DRIVER_FACTORY_H_
#define SHELL_BROWSER_ELECTRON_AUTOFILL_DRIVER_FACTORY_H_

#include <memory>
#include <unordered_map>

#include "base/callback_forward.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "shell/common/api/api.mojom.h"

namespace electron {

class AutofillDriver;

class AutofillDriverFactory
    : public content::WebContentsObserver,
      public content::WebContentsUserData<AutofillDriverFactory> {
 public:
  typedef base::OnceCallback<std::unique_ptr<AutofillDriver>()>
      CreationCallback;

  ~AutofillDriverFactory() override;

  static void BindAutofillDriver(
      mojom::ElectronAutofillDriverAssociatedRequest request,
      content::RenderFrameHost* render_frame_host);

  // 内容：：WebContentsViewer：
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  AutofillDriver* DriverForFrame(content::RenderFrameHost* render_frame_host);
  void AddDriverForFrame(content::RenderFrameHost* render_frame_host,
                         CreationCallback factory_method);
  void DeleteDriverForFrame(content::RenderFrameHost* render_frame_host);

  void CloseAllPopups();

  WEB_CONTENTS_USER_DATA_KEY_DECL();

 private:
  explicit AutofillDriverFactory(content::WebContents* web_contents);
  friend class content::WebContentsUserData<AutofillDriverFactory>;

  std::unordered_map<content::RenderFrameHost*, std::unique_ptr<AutofillDriver>>
      driver_map_;
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_ELECTRON_AUTOFILL_DRIVER_FACTORY_H_
