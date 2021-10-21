// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/process_util.h"

#include "gin/dictionary.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/node_includes.h"

namespace electron {

void EmitWarning(node::Environment* env,
                 const std::string& warning_msg,
                 const std::string& warning_type) {
  v8::HandleScope scope(env->isolate());
  gin::Dictionary process(env->isolate(), env->process_object());

  base::RepeatingCallback<void(base::StringPiece, base::StringPiece,
                               base::StringPiece)>
      emit_warning;
  process.Get("emitWarning", &emit_warning);

  emit_warning.Run(warning_msg, warning_type, "");
}

}  // 命名空间电子
