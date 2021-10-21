// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_COMMON_GIN_CONVERTERS_CONTENT_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_CONTENT_CONVERTER_H_

#include <utility>

#include "content/public/browser/permission_type.h"
#include "content/public/common/referrer.h"
#include "content/public/common/stop_find_action.h"
#include "gin/converter.h"
#include "third_party/blink/public/mojom/choosers/popup_menu.mojom.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom.h"

namespace content {
struct ContextMenuParams;
struct NativeWebKeyboardEvent;
class RenderFrameHost;
class WebContents;
}  // 命名空间内容。

using ContextMenuParamsWithRenderFrameHost =
    std::pair<content::ContextMenuParams, content::RenderFrameHost*>;

namespace gin {

template <>
struct Converter<blink::mojom::MenuItem::Type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::mojom::MenuItem::Type& val);
};

template <>
struct Converter<ContextMenuParamsWithRenderFrameHost> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const ContextMenuParamsWithRenderFrameHost& val);
};

template <>
struct Converter<blink::mojom::PermissionStatus> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::mojom::PermissionStatus* out);
};

template <>
struct Converter<content::PermissionType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::PermissionType& val);
};

template <>
struct Converter<content::StopFindAction> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     content::StopFindAction* out);
};

template <>
struct Converter<content::WebContents*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::WebContents* val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     content::WebContents** out);
};

template <>
struct Converter<content::Referrer> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::Referrer& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     content::Referrer* out);
};

template <>
struct Converter<content::NativeWebKeyboardEvent> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     content::NativeWebKeyboardEvent* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::NativeWebKeyboardEvent& in);
};

}  // 命名空间杜松子酒。

#endif  // SHELL_COMMON_GIN_CONVERTERS_CONTENT_CONVERTER_H_
