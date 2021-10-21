// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/message_port.h"

#include <string>
#include <unordered_set>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "gin/arguments.h"
#include "gin/data_object_builder.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_value_serializer.h"
#include "third_party/blink/public/common/messaging/transferable_message.h"
#include "third_party/blink/public/common/messaging/transferable_message_mojom_traits.h"
#include "third_party/blink/public/mojom/messaging/transferable_message.mojom.h"

namespace electron {

gin::WrapperInfo MessagePort::kWrapperInfo = {gin::kEmbedderNativeGin};

MessagePort::MessagePort() = default;
MessagePort::~MessagePort() {
  if (!IsNeutered()) {
    // 拆卸前先解开。如果出现以下情况，MessagePortDescriptor将会爆炸。
    // 在被拆毁之前，它的底层句柄还没有归还给它。
    Disentangle();
  }
}

// 静电。
gin::Handle<MessagePort> MessagePort::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new MessagePort());
}

void MessagePort::PostMessage(gin::Arguments* args) {
  if (!IsEntangled())
    return;
  DCHECK(!IsNeutered());

  blink::TransferableMessage transferable_message;

  v8::Local<v8::Value> message_value;
  if (!args->GetNext(&message_value)) {
    args->ThrowTypeError("Expected at least one argument to postMessage");
    return;
  }

  electron::SerializeV8Value(args->isolate(), message_value,
                             &transferable_message);

  v8::Local<v8::Value> transferables;
  std::vector<gin::Handle<MessagePort>> wrapped_ports;
  if (args->GetNext(&transferables)) {
    if (!gin::ConvertFromV8(args->isolate(), transferables, &wrapped_ports)) {
      args->ThrowError();
      return;
    }
  }

  // 确保我们没有连接到任何传入的端口。
  for (unsigned i = 0; i < wrapped_ports.size(); ++i) {
    if (wrapped_ports[i].get() == this) {
      gin_helper::ErrorThrower(args->isolate())
          .ThrowError("Port at index " + base::NumberToString(i) +
                      " contains the source port.");
      return;
    }
  }

  bool threw_exception = false;
  transferable_message.ports = MessagePort::DisentanglePorts(
      args->isolate(), wrapped_ports, &threw_exception);
  if (threw_exception)
    return;

  mojo::Message mojo_message = blink::mojom::TransferableMessage::WrapAsMessage(
      std::move(transferable_message));
  connector_->Accept(&mojo_message);
}

void MessagePort::Start() {
  if (!IsEntangled())
    return;

  if (started_)
    return;

  started_ = true;
  if (HasPendingActivity())
    Pin();
  connector_->ResumeIncomingMethodCallProcessing();
}

void MessagePort::Close() {
  if (closed_)
    return;
  if (!IsNeutered()) {
    Disentangle().ReleaseHandle();
    blink::MessagePortDescriptorPair pipe;
    Entangle(pipe.TakePort0());
  }
  closed_ = true;
  if (!HasPendingActivity())
    Unpin();

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> self;
  if (GetWrapper(isolate).ToLocal(&self))
    gin_helper::EmitEvent(isolate, self, "close");
}

void MessagePort::Entangle(blink::MessagePortDescriptor port) {
  DCHECK(port.IsValid());
  DCHECK(!connector_);
  port_ = std::move(port);
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  connector_ = std::make_unique<mojo::Connector>(
      port_.TakeHandleToEntangleWithEmbedder(),
      mojo::Connector::SINGLE_THREADED_SEND,
      base::ThreadTaskRunnerHandle::Get());
  connector_->PauseIncomingMethodCallProcessing();
  connector_->set_incoming_receiver(this);
  connector_->set_connection_error_handler(
      base::BindOnce(&MessagePort::Close, weak_factory_.GetWeakPtr()));
  if (HasPendingActivity())
    Pin();
}

void MessagePort::Entangle(blink::MessagePortChannel channel) {
  Entangle(channel.ReleaseHandle());
}

blink::MessagePortChannel MessagePort::Disentangle() {
  DCHECK(!IsNeutered());
  port_.GiveDisentangledHandle(connector_->PassMessagePipe());
  connector_ = nullptr;
  if (!HasPendingActivity())
    Unpin();
  return blink::MessagePortChannel(std::move(port_));
}

bool MessagePort::HasPendingActivity() const {
  // 该规范指出，纠缠的消息端口应始终被视为。
  // 他们有很强的参考价值。
  // 我们还将规定队列需要打开(如果应用程序丢弃了。
  // 在开始()之前引用端口，那么它就不是真正纠缠在一起的。
  // 因为它是遥不可及的)。
  return started_ && IsEntangled();
}

// 静电。
std::vector<gin::Handle<MessagePort>> MessagePort::EntanglePorts(
    v8::Isolate* isolate,
    std::vector<blink::MessagePortChannel> channels) {
  std::vector<gin::Handle<MessagePort>> wrapped_ports;
  for (auto& port : channels) {
    auto wrapped_port = MessagePort::Create(isolate);
    wrapped_port->Entangle(std::move(port));
    wrapped_ports.emplace_back(wrapped_port);
  }
  return wrapped_ports;
}

// 静电。
std::vector<blink::MessagePortChannel> MessagePort::DisentanglePorts(
    v8::Isolate* isolate,
    const std::vector<gin::Handle<MessagePort>>& ports,
    bool* threw_exception) {
  if (ports.empty())
    return std::vector<blink::MessagePortChannel>();

  std::unordered_set<MessagePort*> visited;

  // 检查传入阵列-如果有任何重复端口或空端口。
  // 或克隆的端口，则抛出错误(根据HTML5规范的8.3.3节)。
  for (unsigned i = 0; i < ports.size(); ++i) {
    auto* port = ports[i].get();
    if (!port || port->IsNeutered() || visited.find(port) != visited.end()) {
      std::string type;
      if (!port)
        type = "null";
      else if (port->IsNeutered())
        type = "already neutered";
      else
        type = "a duplicate";
      gin_helper::ErrorThrower(isolate).ThrowError(
          "Port at index " + base::NumberToString(i) + " is " + type + ".");
      *threw_exception = true;
      return std::vector<blink::MessagePortChannel>();
    }
    visited.insert(port);
  }

  // 传入的端口通过了有效性检查，因此我们可以将其解开。
  std::vector<blink::MessagePortChannel> channels;
  channels.reserve(ports.size());
  for (unsigned i = 0; i < ports.size(); ++i)
    channels.push_back(ports[i]->Disentangle());
  return channels;
}

void MessagePort::Pin() {
  if (!pinned_.IsEmpty())
    return;
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Value> self;
  if (GetWrapper(isolate).ToLocal(&self)) {
    pinned_.Reset(isolate, self);
  }
}

void MessagePort::Unpin() {
  pinned_.Reset();
}

bool MessagePort::Accept(mojo::Message* mojo_message) {
  blink::TransferableMessage message;
  if (!blink::mojom::TransferableMessage::DeserializeFromMessage(
          std::move(*mojo_message), &message)) {
    return false;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);

  auto ports = EntanglePorts(isolate, std::move(message.ports));

  v8::Local<v8::Value> message_value = DeserializeV8Value(isolate, message);

  v8::Local<v8::Object> self;
  if (!GetWrapper(isolate).ToLocal(&self))
    return false;

  auto event = gin::DataObjectBuilder(isolate)
                   .Set("data", message_value)
                   .Set("ports", ports)
                   .Build();
  gin_helper::EmitEvent(isolate, self, "message", event);
  return true;
}

gin::ObjectTemplateBuilder MessagePort::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<MessagePort>::GetObjectTemplateBuilder(isolate)
      .SetMethod("postMessage", &MessagePort::PostMessage)
      .SetMethod("start", &MessagePort::Start)
      .SetMethod("close", &MessagePort::Close);
}

const char* MessagePort::GetTypeName() {
  return "MessagePort";
}

}  // 命名空间电子。

namespace {

using electron::MessagePort;

v8::Local<v8::Value> CreatePair(v8::Isolate* isolate) {
  auto port1 = MessagePort::Create(isolate);
  auto port2 = MessagePort::Create(isolate);
  blink::MessagePortDescriptorPair pipe;
  port1->Entangle(pipe.TakePort0());
  port2->Entangle(pipe.TakePort1());
  return gin::DataObjectBuilder(isolate)
      .Set("port1", port1)
      .Set("port2", port2)
      .Build();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("createPair", &CreatePair);
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_message_port, Initialize)
