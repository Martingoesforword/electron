// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_helper/error_thrower.h"

#include "gin/converter.h"

namespace gin_helper {

ErrorThrower::ErrorThrower(v8::Isolate* isolate) : isolate_(isolate) {}

// 此构造函数即使使用过，也应该很少使用，因为。
// V8：：Isolate：：GetCurrent()使用原子加载，因此有点。
// 调用成本高昂。
ErrorThrower::ErrorThrower() : isolate_(v8::Isolate::GetCurrent()) {}

void ErrorThrower::ThrowError(base::StringPiece err_msg) const {
  Throw(v8::Exception::Error, err_msg);
}

void ErrorThrower::ThrowTypeError(base::StringPiece err_msg) const {
  Throw(v8::Exception::TypeError, err_msg);
}

void ErrorThrower::ThrowRangeError(base::StringPiece err_msg) const {
  Throw(v8::Exception::RangeError, err_msg);
}

void ErrorThrower::ThrowReferenceError(base::StringPiece err_msg) const {
  Throw(v8::Exception::ReferenceError, err_msg);
}

void ErrorThrower::ThrowSyntaxError(base::StringPiece err_msg) const {
  Throw(v8::Exception::SyntaxError, err_msg);
}

void ErrorThrower::Throw(ErrorGenerator gen, base::StringPiece err_msg) const {
  v8::Local<v8::Value> exception = gen(gin::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

}  // 命名空间gin_helper
