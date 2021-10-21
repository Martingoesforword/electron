// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_converters/time_converter.h"

#include "base/time/time.h"
#include "v8/include/v8-date.h"

namespace gin {

v8::Local<v8::Value> Converter<base::Time>::ToV8(v8::Isolate* isolate,
                                                 const base::Time& val) {
  v8::Local<v8::Value> date;
  if (v8::Date::New(isolate->GetCurrentContext(), val.ToJsTime())
          .ToLocal(&date))
    return date;
  else
    return v8::Null(isolate);
}

}  // 命名空间杜松子酒
