// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_

#include <string>

#include "gin/converter.h"

#if defined(OS_WIN)
// C.F.：
// Https://chromium-review.googlesource.com/c/chromium/src/+/3076480。
// REFGUID当前错误地继承其类型。
// 当它应该从：：GUID继承时，从base：：GUID返回。
// 此解决方法可防止在合并CL之前出现生成错误。
#ifdef _REFGUID_DEFINED
#undef REFGUID
#endif
#define REFGUID const ::GUID&
#define _REFGUID_DEFINED

#include <rpc.h>

#include "base/strings/sys_string_conversions.h"
#include "base/win/win_util.h"
#endif

#if defined(OS_WIN)
typedef GUID UUID;
#else
typedef struct {
} UUID;
#endif

namespace gin {

template <>
struct Converter<UUID> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     UUID* out) {
#if defined(OS_WIN)
    std::string guid;
    if (!gin::ConvertFromV8(isolate, val, &guid))
      return false;

    UUID uid;

    if (!guid.empty()) {
      if (guid[0] == '{' && guid[guid.length() - 1] == '}') {
        guid = guid.substr(1, guid.length() - 2);
      }
      unsigned char* uid_cstr = (unsigned char*)guid.c_str();
      RPC_STATUS result = UuidFromStringA(uid_cstr, &uid);
      if (result == RPC_S_INVALID_STRING_UUID) {
        return false;
      } else {
        *out = uid;
        return true;
      }
    }
    return false;
#else
    return false;
#endif
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, UUID val) {
#if defined(OS_WIN)
    const GUID GUID_NULL = {};
    if (val == GUID_NULL) {
      return StringToV8(isolate, "");
    } else {
      std::wstring uid = base::win::WStringFromGUID(val);
      return StringToV8(isolate, base::SysWideToUTF8(uid));
    }
#else
    return v8::Undefined(isolate);
#endif
  }
};

}  // 命名空间杜松子酒。

#endif  // Shell_COMMON_GIN_CONVERTERS_GUID_CONFERTER_H_
