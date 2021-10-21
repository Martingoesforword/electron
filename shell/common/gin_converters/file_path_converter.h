// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_FILE_PATH_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_FILE_PATH_CONVERTER_H_

#include "base/files/file_path.h"
#include "gin/converter.h"
#include "shell/common/gin_converters/std_converter.h"

namespace gin {

template <>
struct Converter<base::FilePath> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::FilePath& val) {
    return Converter<base::FilePath::StringType>::ToV8(isolate, val.value());
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::FilePath* out) {
    if (val->IsNull())
      return true;

    v8::String::Value str(isolate, val);
    if (str.length() == 0) {
      *out = base::FilePath();
      return true;
    }

    base::FilePath::StringType path;
    if (Converter<base::FilePath::StringType>::FromV8(isolate, val, &path)) {
      *out = base::FilePath(path);
      return true;
    } else {
      return false;
    }
  }
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_CONVERTERS_FILE_PATH_CONVERTER_H_
