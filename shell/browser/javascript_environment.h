// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_
#define SHELL_BROWSER_JAVASCRIPT_ENVIRONMENT_H_

#include <memory>

#include "base/macros.h"
#include "gin/public/isolate_holder.h"
#include "uv.h"  // NOLINT(BUILD/INCLUDE_DIRECTORY)。
#include "v8/include/v8-locker.h"

namespace node {
class Environment;
class MultiIsolatePlatform;
}  // 命名空间节点。

namespace electron {

class MicrotasksRunner;
// 自动管理V8隔离和上下文。
class JavascriptEnvironment {
 public:
  explicit JavascriptEnvironment(uv_loop_t* event_loop);
  ~JavascriptEnvironment();

  void OnMessageLoopCreated();
  void OnMessageLoopDestroying();

  node::MultiIsolatePlatform* platform() const { return platform_; }
  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Context> context() const {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }

  static v8::Isolate* GetIsolate();

 private:
  v8::Isolate* Initialize(uv_loop_t* event_loop);
  // 出口漏水了。
  node::MultiIsolatePlatform* platform_;

  v8::Isolate* isolate_;
  gin::IsolateHolder isolate_holder_;
  v8::Locker locker_;
  v8::Global<v8::Context> context_;

  std::unique_ptr<MicrotasksRunner> microtasks_runner_;

  DISALLOW_COPY_AND_ASSIGN(JavascriptEnvironment);
};

// 自动管理节点环境。
class NodeEnvironment {
 public:
  explicit NodeEnvironment(node::Environment* env);
  ~NodeEnvironment();

  node::Environment* env() { return env_; }

 private:
  node::Environment* env_;

  DISALLOW_COPY_AND_ASSIGN(NodeEnvironment);
};

}  // 命名空间电子。

#endif  // Shell_Browser_JavaScript_Environment_H_
