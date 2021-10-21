// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_EVENT_H_
#define SHELL_BROWSER_API_EVENT_H_

#include "electron/shell/common/api/api.mojom.h"
#include "gin/handle.h"
#include "gin/wrappable.h"

namespace gin_helper {

class Event : public gin::Wrappable<Event> {
 public:
  using InvokeCallback = electron::mojom::ElectronBrowser::InvokeCallback;

  static gin::WrapperInfo kWrapperInfo;

  static gin::Handle<Event> Create(v8::Isolate* isolate);

  // 传递要调用的回调。
  void SetCallback(InvokeCallback callback);

  // Event.PreventDefault()。
  void PreventDefault(v8::Isolate* isolate);

  // Event.sendReply(Value)，用于回复同步消息和。
  // ‘Invoke`调用。
  bool SendReply(v8::Isolate* isolate, v8::Local<v8::Value> result);

 protected:
  Event();
  ~Event() override;

  // 杜松子酒：：可包装的：
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 private:
  // 同步消息的复制器。
  InvokeCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(Event);
};

}  // 命名空间gin_helper。

#endif  // Shell_Browser_API_Event_H_
