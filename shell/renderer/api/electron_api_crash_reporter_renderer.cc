// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "base/bind.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

#if !defined(MAS_BUILD)
#include "shell/common/crash_keys.h"
#endif

namespace {

v8::Local<v8::Value> GetParameters(v8::Isolate* isolate) {
  std::map<std::string, std::string> keys;
#if !defined(MAS_BUILD)
  electron::crash_keys::GetCrashKeys(&keys);
#endif
  return gin::ConvertToV8(isolate, keys);
}

#if defined(MAS_BUILD)
void SetCrashKeyStub(const std::string& key, const std::string& value) {}
void ClearCrashKeyStub(const std::string& key) {}
#endif

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
#if defined(MAS_BUILD)
  dict.SetMethod("addExtraParameter", &SetCrashKeyStub);
  dict.SetMethod("removeExtraParameter", &ClearCrashKeyStub);
#else
  dict.SetMethod("addExtraParameter", &electron::crash_keys::SetCrashKey);
  dict.SetMethod("removeExtraParameter", &electron::crash_keys::ClearCrashKey);
#endif
  dict.SetMethod("getParameters", &GetParameters);
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_renderer_crash_reporter, Initialize)
