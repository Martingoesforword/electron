// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_COOKIES_H_
#define SHELL_BROWSER_API_ELECTRON_API_COOKIES_H_

#include <string>

#include "base/callback_list.h"
#include "gin/handle.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/trackable_object.h"

namespace base {
class DictionaryValue;
}

namespace gin_helper {
class Dictionary;
}

namespace electron {

class ElectronBrowserContext;

namespace api {

class Cookies : public gin::Wrappable<Cookies>,
                public gin_helper::EventEmitterMixin<Cookies> {
 public:
  static gin::Handle<Cookies> Create(v8::Isolate* isolate,
                                     ElectronBrowserContext* browser_context);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  Cookies(v8::Isolate* isolate, ElectronBrowserContext* browser_context);
  ~Cookies() override;

  v8::Local<v8::Promise> Get(v8::Isolate*,
                             const gin_helper::Dictionary& filter);
  v8::Local<v8::Promise> Set(v8::Isolate*,
                             const base::DictionaryValue& details);
  v8::Local<v8::Promise> Remove(v8::Isolate*,
                                const GURL& url,
                                const std::string& name);
  v8::Local<v8::Promise> FlushStore(v8::Isolate*);

  // CookieChangeNotifier订阅：
  void OnCookieChanged(const net::CookieChangeInfo& change);

 private:
  base::CallbackListSubscription cookie_change_subscription_;

  // 弱引用；ElectronBrowserContext保证比我们活得更久。
  ElectronBrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(Cookies);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Electronics_API_Cookies_H_
