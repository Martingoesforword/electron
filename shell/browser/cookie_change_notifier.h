// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_COOKIE_CHANGE_NOTIFIER_H_
#define SHELL_BROWSER_COOKIE_CHANGE_NOTIFIER_H_

#include "base/callback_list.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

namespace electron {

class ElectronBrowserContext;

// 在UI线程上发送Cookie更改通知。
class CookieChangeNotifier : public network::mojom::CookieChangeListener {
 public:
  explicit CookieChangeNotifier(ElectronBrowserContext* browser_context);
  ~CookieChangeNotifier() override;

  // 注册需要在任何Cookie存储更改时通知的回调。
  base::CallbackListSubscription RegisterCookieChangeCallback(
      const base::RepeatingCallback<void(const net::CookieChangeInfo& change)>&
          cb);

 private:
  void StartListening();
  void OnConnectionError();

  // Network：：mojom：：CookieChangeListener实现。
  void OnCookieChange(const net::CookieChangeInfo& change) override;

  ElectronBrowserContext* browser_context_;
  base::RepeatingCallbackList<void(const net::CookieChangeInfo& change)>
      cookie_change_sub_list_;

  mojo::Receiver<network::mojom::CookieChangeListener> receiver_;

  DISALLOW_COPY_AND_ASSIGN(CookieChangeNotifier);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Cookie_Change_Notifier_H_
