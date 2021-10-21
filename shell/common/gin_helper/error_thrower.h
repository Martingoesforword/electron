// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_ERROR_THROWER_H_
#define SHELL_COMMON_GIN_HELPER_ERROR_THROWER_H_

#include "base/strings/string_piece.h"
#include "v8/include/v8.h"

namespace gin_helper {

class ErrorThrower {
 public:
  explicit ErrorThrower(v8::Isolate* isolate);
  ErrorThrower();
  ~ErrorThrower() = default;

  void ThrowError(base::StringPiece err_msg) const;
  void ThrowTypeError(base::StringPiece err_msg) const;
  void ThrowRangeError(base::StringPiece err_msg) const;
  void ThrowReferenceError(base::StringPiece err_msg) const;
  void ThrowSyntaxError(base::StringPiece err_msg) const;

  v8::Isolate* isolate() const { return isolate_; }

 private:
  using ErrorGenerator =
      v8::Local<v8::Value> (*)(v8::Local<v8::String> err_msg);
  void Throw(ErrorGenerator gen, base::StringPiece err_msg) const;

  v8::Isolate* isolate_;
};

}  // 命名空间gin_helper。

#endif  // SHELL_COMMON_GIN_HELPER_ERROR_SROWER_H_
