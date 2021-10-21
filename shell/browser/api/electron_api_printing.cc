// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "base/threading/thread_restrictions.h"
#include "chrome/browser/browser_process.h"
#include "gin/converter.h"
#include "printing/buildflags/buildflags.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "printing/backend/print_backend.h"
#endif

namespace gin {

#if BUILDFLAG(ENABLE_PRINTING)
template <>
struct Converter<printing::PrinterBasicInfo> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const printing::PrinterBasicInfo& val) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.Set("name", val.printer_name);
    dict.Set("displayName", val.display_name);
    dict.Set("description", val.printer_description);
    dict.Set("status", val.printer_status);
    dict.Set("isDefault", val.is_default ? true : false);
    dict.Set("options", val.options);
    return dict.GetHandle();
  }
};
#endif

}  // 命名空间杜松子酒。

namespace electron {

namespace api {

#if BUILDFLAG(ENABLE_PRINTING)
printing::PrinterList GetPrinterList() {
  printing::PrinterList printers;
  auto print_backend = printing::PrintBackend::CreateInstance(
      g_browser_process->GetApplicationLocale());
  {
    // TODO(Deepak1556)：不推荐使用此API，而支持。
    // 异步版本并发布非阻塞任务调用。
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    printing::mojom::ResultCode code =
        print_backend->EnumeratePrinters(&printers);
    if (code != printing::mojom::ResultCode::kSuccess)
      LOG(INFO) << "Failed to enumerate printers";
  }
  return printers;
}
#endif

}  // 命名空间API。

}  // 命名空间电子。

namespace {

#if BUILDFLAG(ENABLE_PRINTING)
using electron::api::GetPrinterList;
#endif

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
#if BUILDFLAG(ENABLE_PRINTING)
  dict.SetMethod("getPrinterList", base::BindRepeating(&GetPrinterList));
#endif
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_printing, Initialize)
