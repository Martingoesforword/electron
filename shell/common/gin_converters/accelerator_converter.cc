// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_converters/accelerator_converter.h"

#include <string>

#include "shell/browser/ui/accelerator_util.h"

namespace gin {

// 静电。
bool Converter<ui::Accelerator>::FromV8(v8::Isolate* isolate,
                                        v8::Local<v8::Value> val,
                                        ui::Accelerator* out) {
  std::string keycode;
  if (!ConvertFromV8(isolate, val, &keycode))
    return false;
  return accelerator_util::StringToAccelerator(keycode, out);
}

}  // 命名空间杜松子酒
