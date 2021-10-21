// 版权所有(C)2015年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_WIN_SCOPED_HSTRING_H_
#define SHELL_BROWSER_WIN_SCOPED_HSTRING_H_

#include <hstring.h>
#include <windows.h>

#include <string>

#include "base/macros.h"

namespace electron {

class ScopedHString {
 public:
  // 复制自|源|。
  explicit ScopedHString(const wchar_t* source);
  explicit ScopedHString(const std::wstring& source);
  // 创建空字符串。
  ScopedHString();
  ~ScopedHString();

  // 设置为|源|。
  void Reset();
  void Reset(const wchar_t* source);
  void Reset(const std::wstring& source);

  // 返回字符串。
  operator HSTRING() const { return str_; }

  // 是否创建了字符串。
  bool success() const { return str_; }

 private:
  HSTRING str_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ScopedHString);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Win_Scope_HSTRING_H_
