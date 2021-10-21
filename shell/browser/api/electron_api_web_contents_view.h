// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_H_
#define SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_H_

#include "content/public/browser/web_contents_observer.h"
#include "shell/browser/api/electron_api_view.h"

namespace gin_helper {
class Dictionary;
}

namespace electron {

namespace api {

class WebContents;

class WebContentsView : public View, public content::WebContentsObserver {
 public:
  // 创建WebContentsView的新实例。
  static gin::Handle<WebContentsView> Create(
      v8::Isolate* isolate,
      const gin_helper::Dictionary& web_preferences);

  // 返回缓存的构造函数。
  static v8::Local<v8::Function> GetConstructor(v8::Isolate* isolate);

  // GINE_HELPER：：Wrappable。
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // 公共接口。
  gin::Handle<WebContents> GetWebContents(v8::Isolate* isolate);

 protected:
  // 采用现有的WebContents。
  WebContentsView(v8::Isolate* isolate, gin::Handle<WebContents> web_contents);
  ~WebContentsView() override;

  // 内容：：WebContentsViewer：
  void WebContentsDestroyed() override;

 private:
  static gin_helper::WrappableBase* New(
      gin_helper::Arguments* args,
      const gin_helper::Dictionary& web_preferences);

  // 请保留对V8包装器的引用。
  v8::Global<v8::Value> web_contents_;
  api::WebContents* api_web_contents_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsView);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_WEB_CONTENTS_VIEW_H_
