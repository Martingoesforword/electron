// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在LICENSE.Cr文件中找到。

#ifndef SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_
#define SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_

#include "gin/arguments.h"

namespace gin_helper {

// 为gin：：Arguments类提供其他API。
class Arguments : public gin::Arguments {
 public:
  // 获取下一个参数，如果转换为T失败，则状态不变。
  // 
  // 这与gin：：Arguments：：GetNext不同，后者总是将。
  // |NEXT_|转换是否成功的计数器。
  template <typename T>
  bool GetNext(T* out) {
    v8::Local<v8::Value> val = PeekNext();
    if (val.IsEmpty())
      return false;
    if (!gin::ConvertFromV8(isolate(), val, out))
      return false;
    Skip();
    return true;
  }

  // 将V8值转换为布尔值时，GIN始终返回TRUE，我们不希望。
  // 解析参数时的此行为。
  bool GetNext(bool* out) {
    v8::Local<v8::Value> val = PeekNext();
    if (val.IsEmpty() || !val->IsBoolean())
      return false;
    *out = val->BooleanValue(isolate());
    Skip();
    return true;
  }

  // 引发带有自定义错误消息的错误。
  void ThrowError() const;
  void ThrowError(base::StringPiece message) const;

 private:
  // 不得添加任何数据成员。
};

}  // 命名空间gin_helper。

#endif  // Shell_COMMON_GIN_HELPER_ARGUMENTS_H_
