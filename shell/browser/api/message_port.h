// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_MESSAGE_PORT_H_
#define SHELL_BROWSER_API_MESSAGE_PORT_H_

#include <memory>
#include <vector>

#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/connector.h"
#include "mojo/public/cpp/bindings/message.h"
#include "third_party/blink/public/common/messaging/message_port_channel.h"
#include "third_party/blink/public/common/messaging/message_port_descriptor.h"

namespace gin {
class Arguments;
template <typename T>
class Handle;
}  // 命名空间杜松子酒。

namespace electron {

// Blink：：MessagePort的非闪烁版本。
class MessagePort : public gin::Wrappable<MessagePort>, mojo::MessageReceiver {
 public:
  ~MessagePort() override;
  static gin::Handle<MessagePort> Create(v8::Isolate* isolate);

  void PostMessage(gin::Arguments* args);
  void Start();
  void Close();

  void Entangle(blink::MessagePortDescriptor port);
  void Entangle(blink::MessagePortChannel channel);

  blink::MessagePortChannel Disentangle();

  bool IsEntangled() const { return !closed_ && !IsNeutered(); }
  bool IsNeutered() const { return !connector_ || !connector_->is_valid(); }

  static std::vector<gin::Handle<MessagePort>> EntanglePorts(
      v8::Isolate* isolate,
      std::vector<blink::MessagePortChannel> channels);

  static std::vector<blink::MessagePortChannel> DisentanglePorts(
      v8::Isolate* isolate,
      const std::vector<gin::Handle<MessagePort>>& ports,
      bool* threw_exception);

  // 杜松子酒：：可包装的。
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  static gin::WrapperInfo kWrapperInfo;
  const char* GetTypeName() override;

 private:
  MessagePort();

  // MessagePort的闪烁版本使用了非常好的“ActiveScriptWrapper”
  // 类，该类通过v8嵌入器钩子将对象保持为活动状态。
  // GC生命周期：请参阅。
  // Https://source.chromium.org/chromium/chromium/src/+/master:third_party/blink/renderer/platform/heap/thread_state.cc；l=258；drc=b892cf58e162a8f66cd76d7472f129fe0fb6a7d1。
  // 我们没有这种奢侈，所以我们粗暴地使用V8：：GLOBAL来完成。
  // 类似的东西。关键的是，每当。
  // “HasPendingActivity()”更改，我们必须将Pin()或Unpin()调用为。
  // 恰如其分。
  bool HasPendingActivity() const;
  void Pin();
  void Unpin();

  // MOJO：：MessageReceiver。
  bool Accept(mojo::Message* mojo_message) override;

  std::unique_ptr<mojo::Connector> connector_;
  bool started_ = false;
  bool closed_ = false;

  v8::Global<v8::Value> pinned_;

  // 此类拥有的内部端口。句柄本身被移动到。
  // |Connector_|处于纠缠状态。
  blink::MessagePortDescriptor port_;

  base::WeakPtrFactory<MessagePort> weak_factory_{this};
};

}  // 命名空间电子。

#endif  // Shell_Browser_API_Message_Port_H_
