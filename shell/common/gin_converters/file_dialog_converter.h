// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_FILE_DIALOG_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_FILE_DIALOG_CONVERTER_H_

#include "gin/converter.h"
#include "shell/browser/ui/file_dialog.h"

namespace gin {

template <>
struct Converter<file_dialog::Filter> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const file_dialog::Filter& in);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     file_dialog::Filter* out);
};

template <>
struct Converter<file_dialog::DialogSettings> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const file_dialog::DialogSettings& in);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     file_dialog::DialogSettings* out);
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_CONVERTERS_FILE_DIALOG_CONVERTER_H_
