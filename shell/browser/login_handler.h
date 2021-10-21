// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_LOGIN_HANDLER_H_
#define SHELL_BROWSER_LOGIN_HANDLER_H_

#include "base/values.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}

namespace gin {
class Arguments;
}

namespace electron {

// 处理HTTP基本身份验证。
class LoginHandler : public content::LoginDelegate,
                     public content::WebContentsObserver {
 public:
  LoginHandler(const net::AuthChallengeInfo& auth_info,
               content::WebContents* web_contents,
               bool is_main_frame,
               const GURL& url,
               scoped_refptr<net::HttpResponseHeaders> response_headers,
               bool first_auth_attempt,
               LoginAuthRequiredCallback auth_required_callback);
  ~LoginHandler() override;

 private:
  void EmitEvent(net::AuthChallengeInfo auth_info,
                 bool is_main_frame,
                 const GURL& url,
                 scoped_refptr<net::HttpResponseHeaders> response_headers,
                 bool first_auth_attempt);
  void CallbackFromJS(gin::Arguments* args);

  LoginAuthRequiredCallback auth_required_callback_;

  base::WeakPtrFactory<LoginHandler> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(LoginHandler);
};

}  // 命名空间电子。

#endif  // Shell_Browser_LOGIN_HANDLER_H_
