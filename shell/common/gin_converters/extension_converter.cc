// 版权所有(C)2019 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_converters/extension_converter.h"

#include "extensions/common/extension.h"
#include "gin/dictionary.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"

namespace gin {

// 静电。
v8::Local<v8::Value> Converter<const extensions::Extension*>::ToV8(
    v8::Isolate* isolate,
    const extensions::Extension* extension) {
  auto dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("id", extension->id());
  dict.Set("name", extension->name());
  dict.Set("path", extension->path());
  dict.Set("url", extension->url());
  dict.Set("version", extension->VersionString());
  dict.Set("manifest", *(extension->manifest()->value()));

  return gin::ConvertToV8(isolate, dict);
}

}  // 命名空间杜松子酒
