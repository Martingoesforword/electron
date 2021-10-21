// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_BASE_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_BASE_CONVERTER_H_

#include "base/process/kill.h"
#include "gin/converter.h"

namespace gin {

template <>
struct Converter<base::TerminationStatus> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::TerminationStatus& status) {
    switch (status) {
      case base::TERMINATION_STATUS_NORMAL_TERMINATION:
        return gin::ConvertToV8(isolate, "clean-exit");
      case base::TERMINATION_STATUS_ABNORMAL_TERMINATION:
        return gin::ConvertToV8(isolate, "abnormal-exit");
      case base::TERMINATION_STATUS_PROCESS_WAS_KILLED:
        return gin::ConvertToV8(isolate, "killed");
      case base::TERMINATION_STATUS_PROCESS_CRASHED:
        return gin::ConvertToV8(isolate, "crashed");
      case base::TERMINATION_STATUS_STILL_RUNNING:
        return gin::ConvertToV8(isolate, "still-running");
      case base::TERMINATION_STATUS_LAUNCH_FAILED:
        return gin::ConvertToV8(isolate, "launch-failed");
      case base::TERMINATION_STATUS_OOM:
        return gin::ConvertToV8(isolate, "oom");
#if defined(OS_WIN)
      case base::TERMINATION_STATUS_INTEGRITY_FAILURE:
        return gin::ConvertToV8(isolate, "integrity-failure");
#endif
      case base::TERMINATION_STATUS_MAX_ENUM:
        NOTREACHED();
        return gin::ConvertToV8(isolate, "");
    }
    NOTREACHED();
    return gin::ConvertToV8(isolate, "");
  }
};

}  // 命名空间杜松子酒。

#endif  // Shell_COMMON_GIN_CONVERTERS_BASE_转换器_H_
