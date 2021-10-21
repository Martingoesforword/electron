// 版权所有(C)2021 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "base/dcheck_is_on.h"
#include "base/logging.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8.h"

#if DCHECK_IS_ON()
namespace {

void Log(int severity, std::string text) {
  switch (severity) {
    case logging::LOGGING_VERBOSE:
      VLOG(1) << text;
      break;
    case logging::LOGGING_INFO:
      LOG(INFO) << text;
      break;
    case logging::LOGGING_WARNING:
      LOG(WARNING) << text;
      break;
    case logging::LOGGING_ERROR:
      LOG(ERROR) << text;
      break;
    case logging::LOGGING_FATAL:
      LOG(FATAL) << text;
      break;
    default:
      LOG(ERROR) << "Unrecognized severity: " << severity;
      break;
  }
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("log", &Log);
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_testing, Initialize)
#endif
