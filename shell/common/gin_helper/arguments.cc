// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在LICENSE.Cr文件中找到。

#include "shell/common/gin_helper/arguments.h"

#include "v8/include/v8-exception.h"
namespace gin_helper {

void Arguments::ThrowError() const {
  // GIN在转换失败时前进|NEXT_|计数器，而我们没有，所以我们。
  // 必须手动推进此处的计数器，才能使GIN报告错误。
  // 正确的索引。
  const_cast<Arguments*>(this)->Skip();
  gin::Arguments::ThrowError();
}

void Arguments::ThrowError(base::StringPiece message) const {
  isolate()->ThrowException(
      v8::Exception::Error(gin::StringToV8(isolate(), message)));
}

}  // 命名空间gin_helper
