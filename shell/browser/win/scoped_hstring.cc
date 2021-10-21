// 版权所有(C)2015年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#include "shell/browser/win/scoped_hstring.h"

#include <winstring.h>

namespace electron {

ScopedHString::ScopedHString(const wchar_t* source) {
  Reset(source);
}

ScopedHString::ScopedHString(const std::wstring& source) {
  Reset(source);
}

ScopedHString::ScopedHString() = default;

ScopedHString::~ScopedHString() {
  Reset();
}

void ScopedHString::Reset() {
  if (str_) {
    WindowsDeleteString(str_);
    str_ = nullptr;
  }
}

void ScopedHString::Reset(const wchar_t* source) {
  Reset();
  WindowsCreateString(source, wcslen(source), &str_);
}

void ScopedHString::Reset(const std::wstring& source) {
  Reset();
  WindowsCreateString(source.c_str(), source.length(), &str_);
}

}  // 命名空间电子
