// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_helper/event_emitter_caller.h"

#include "shell/common/gin_helper/locker.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "shell/common/node_includes.h"

namespace gin_helper {

namespace internal {

v8::Local<v8::Value> CallMethodWithArgs(v8::Isolate* isolate,
                                        v8::Local<v8::Object> obj,
                                        const char* method,
                                        ValueVector* args) {
  // 运行JavaScript后执行微任务检查点。
  gin_helper::MicrotasksScope microtasks_scope(isolate, true);
  // 使用node：：MakeCallback调用回调，它也将运行挂起。
  // Node.js中的任务。
  v8::MaybeLocal<v8::Value> ret = node::MakeCallback(
      isolate, obj, method, args->size(), args->data(), {0, 0});
  // 如果JS函数抛出异常(不返回值)，则结果。
  // 的MakeCallback将为空，因此ToLocal将为false，在此。
  // 如果我们需要返回“false”，因为这表明事件发射器返回了。
  // 不处理事件。
  v8::Local<v8::Value> localRet;
  if (ret.ToLocal(&localRet)) {
    return localRet;
  }
  return v8::Boolean::New(isolate, false);
}

}  // 命名空间内部。

}  // 命名空间gin_helper
