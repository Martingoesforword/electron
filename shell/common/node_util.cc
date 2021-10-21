// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/node_util.h"

#include "base/logging.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace util {

v8::MaybeLocal<v8::Value> CompileAndCall(
    v8::Local<v8::Context> context,
    const char* id,
    std::vector<v8::Local<v8::String>>* parameters,
    std::vector<v8::Local<v8::Value>>* arguments,
    node::Environment* optional_env) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::TryCatch try_catch(isolate);
  v8::MaybeLocal<v8::Function> compiled =
      node::native_module::NativeModuleEnv::LookupAndCompile(
          context, id, parameters, optional_env);
  if (compiled.IsEmpty()) {
    return v8::MaybeLocal<v8::Value>();
  }
  v8::Local<v8::Function> fn = compiled.ToLocalChecked().As<v8::Function>();
  v8::MaybeLocal<v8::Value> ret = fn->Call(
      context, v8::Null(isolate), arguments->size(), arguments->data());
  // 只有当某些事情出了可怕的问题时，才会发现这一点。
  // 电子剧本包装在webpack的一次尝试{}捕捉{}中。
  if (try_catch.HasCaught()) {
    LOG(ERROR) << "Failed to CompileAndCall electron script: " << id;
  }
  return ret;
}

}  // 命名空间实用程序。

}  // 命名空间电子
