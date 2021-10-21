// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_web_contents.h"

#include <limits>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/containers/id_map.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/current_thread.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/views/eye_dropper/eye_dropper.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/security_state/content/content_utils.h"
#include "components/security_state/core/security_state.h"
#include "content/browser/renderer_host/frame_tree_node.h"  // 点名检查。
#include "content/browser/renderer_host/render_frame_host_manager.h"  // 点名检查。
#include "content/browser/renderer_host/render_widget_host_impl.h"  // 点名检查。
#include "content/browser/renderer_host/render_widget_host_view_base.h"  // 点名检查。
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/download_request_utils.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/security_style_explanation.h"
#include "content/public/browser/security_style_explanations.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer_type_converters.h"
#include "content/public/common/webplugininfo.h"
#include "electron/buildflags/buildflags.h"
#include "electron/shell/common/api/api.mojom.h"
#include "gin/arguments.h"
#include "gin/data_object_builder.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "ppapi/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "printing/print_job_constants.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/browser/api/electron_api_browser_window.h"
#include "shell/browser/api/electron_api_debugger.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/api/message_port.h"
#include "shell/browser/browser.h"
#include "shell/browser/child_web_contents_tracker.h"
#include "shell/browser/electron_autofill_driver_factory.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/electron_javascript_dialog_manager.h"
#include "shell/browser/electron_navigation_throttle.h"
#include "shell/browser/file_select_helper.h"
#include "shell/browser/native_window.h"
#include "shell/browser/session_preferences.h"
#include "shell/browser/ui/drag_util.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/browser/web_contents_permission_helper.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/browser/web_contents_zoom_controller.h"
#include "shell/browser/web_view_guest_delegate.h"
#include "shell/browser/web_view_manager.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/color_util.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/base_converter.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/language_util.h"
#include "shell/common/mouse_util.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/common/process_util.h"
#include "shell/common/v8_value_serializer.h"
#include "storage/browser/file_system/isolated_context.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/messaging/transferable_message_mojom_traits.h"
#include "third_party/blink/public/common/page/page_zoom.h"
#include "third_party/blink/public/mojom/frame/find_in_page.mojom.h"
#include "third_party/blink/public/mojom/frame/fullscreen.mojom.h"
#include "third_party/blink/public/mojom/messaging/transferable_message.mojom.h"
#include "third_party/blink/public/mojom/renderer_preferences.mojom.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/cursor/mojom/cursor_type.mojom-shared.h"
#include "ui/display/screen.h"
#include "ui/events/base_event_utils.h"

#if BUILDFLAG(ENABLE_OSR)
#include "shell/browser/osr/osr_render_widget_host_view.h"
#include "shell/browser/osr/osr_web_contents_view.h"
#endif

#if !defined(OS_MAC)
#include "ui/aura/window.h"
#else
#include "ui/base/cocoa/defaults_utils.h"
#endif

#if defined(OS_LINUX)
#include "ui/views/linux_ui/linux_ui.h"
#endif

#if defined(OS_LINUX) || defined(OS_WIN)
#include "ui/gfx/font_render_params.h"
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/browser/script_executor.h"
#include "extensions/browser/view_type_utils.h"
#include "extensions/common/mojom/view_type.mojom.h"
#include "shell/browser/extensions/electron_extension_web_contents_observer.h"
#endif

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/printing/browser/print_manager_utils.h"
#include "printing/backend/print_backend.h"  // 点名检查。
#include "printing/mojom/print.mojom.h"
#include "shell/browser/printing/print_preview_message_handler.h"
#include "shell/browser/printing/print_view_manager_electron.h"

#if defined(OS_WIN)
#include "printing/backend/win_helper.h"
#endif
#endif

#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
#include "chrome/browser/picture_in_picture/picture_in_picture_window_manager.h"
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "components/pdf/browser/pdf_web_contents_helper.h"  // 点名检查。
#include "shell/browser/electron_pdf_web_contents_helper_client.h"
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
#include "content/public/browser/plugin_service.h"
#endif

#ifndef MAS_BUILD
#include "chrome/browser/hang_monitor/hang_crash_dump.h"  // 点名检查。
#endif

namespace gin {

#if BUILDFLAG(ENABLE_PRINTING)
template <>
struct Converter<printing::mojom::MarginType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     printing::mojom::MarginType* out) {
    std::string type;
    if (ConvertFromV8(isolate, val, &type)) {
      if (type == "default") {
        *out = printing::mojom::MarginType::kDefaultMargins;
        return true;
      }
      if (type == "none") {
        *out = printing::mojom::MarginType::kNoMargins;
        return true;
      }
      if (type == "printableArea") {
        *out = printing::mojom::MarginType::kPrintableAreaMargins;
        return true;
      }
      if (type == "custom") {
        *out = printing::mojom::MarginType::kCustomMargins;
        return true;
      }
    }
    return false;
  }
};

template <>
struct Converter<printing::mojom::DuplexMode> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     printing::mojom::DuplexMode* out) {
    std::string mode;
    if (ConvertFromV8(isolate, val, &mode)) {
      if (mode == "simplex") {
        *out = printing::mojom::DuplexMode::kSimplex;
        return true;
      }
      if (mode == "longEdge") {
        *out = printing::mojom::DuplexMode::kLongEdge;
        return true;
      }
      if (mode == "shortEdge") {
        *out = printing::mojom::DuplexMode::kShortEdge;
        return true;
      }
    }
    return false;
  }
};

#endif

template <>
struct Converter<WindowOpenDisposition> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   WindowOpenDisposition val) {
    std::string disposition = "other";
    switch (val) {
      case WindowOpenDisposition::CURRENT_TAB:
        disposition = "default";
        break;
      case WindowOpenDisposition::NEW_FOREGROUND_TAB:
        disposition = "foreground-tab";
        break;
      case WindowOpenDisposition::NEW_BACKGROUND_TAB:
        disposition = "background-tab";
        break;
      case WindowOpenDisposition::NEW_POPUP:
      case WindowOpenDisposition::NEW_WINDOW:
        disposition = "new-window";
        break;
      case WindowOpenDisposition::SAVE_TO_DISK:
        disposition = "save-to-disk";
        break;
      default:
        break;
    }
    return gin::ConvertToV8(isolate, disposition);
  }
};

template <>
struct Converter<content::SavePageType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     content::SavePageType* out) {
    std::string save_type;
    if (!ConvertFromV8(isolate, val, &save_type))
      return false;
    save_type = base::ToLowerASCII(save_type);
    if (save_type == "htmlonly") {
      *out = content::SAVE_PAGE_TYPE_AS_ONLY_HTML;
    } else if (save_type == "htmlcomplete") {
      *out = content::SAVE_PAGE_TYPE_AS_COMPLETE_HTML;
    } else if (save_type == "mhtml") {
      *out = content::SAVE_PAGE_TYPE_AS_MHTML;
    } else {
      return false;
    }
    return true;
  }
};

template <>
struct Converter<electron::api::WebContents::Type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   electron::api::WebContents::Type val) {
    using Type = electron::api::WebContents::Type;
    std::string type;
    switch (val) {
      case Type::kBackgroundPage:
        type = "backgroundPage";
        break;
      case Type::kBrowserWindow:
        type = "window";
        break;
      case Type::kBrowserView:
        type = "browserView";
        break;
      case Type::kRemote:
        type = "remote";
        break;
      case Type::kWebView:
        type = "webview";
        break;
      case Type::kOffScreen:
        type = "offscreen";
        break;
      default:
        break;
    }
    return gin::ConvertToV8(isolate, type);
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::api::WebContents::Type* out) {
    using Type = electron::api::WebContents::Type;
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "backgroundPage") {
      *out = Type::kBackgroundPage;
    } else if (type == "browserView") {
      *out = Type::kBrowserView;
    } else if (type == "webview") {
      *out = Type::kWebView;
#if BUILDFLAG(ENABLE_OSR)
    } else if (type == "offscreen") {
      *out = Type::kOffScreen;
#endif
    } else {
      return false;
    }
    return true;
  }
};

template <>
struct Converter<scoped_refptr<content::DevToolsAgentHost>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const scoped_refptr<content::DevToolsAgentHost>& val) {
    gin_helper::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("id", val->GetId());
    dict.Set("url", val->GetURL().spec());
    return dict.GetHandle();
  }
};

}  // 命名空间杜松子酒。

namespace electron {

namespace api {

namespace {

base::IDMap<WebContents*>& GetAllWebContents() {
  static base::NoDestructor<base::IDMap<WebContents*>> s_all_web_contents;
  return *s_all_web_contents;
}

// 在CapturePage完成时调用。
void OnCapturePageDone(gin_helper::Promise<gfx::Image> promise,
                       const SkBitmap& bitmap) {
  // 破解以在捕获的图像中启用透明度。
  promise.Resolve(gfx::Image::CreateFrom1xBitmap(bitmap));
}

absl::optional<base::TimeDelta> GetCursorBlinkInterval() {
#if defined(OS_MAC)
  base::TimeDelta interval;
  if (ui::TextInsertionCaretBlinkPeriod(&interval))
    return interval;
#elif defined(OS_LINUX)
  if (auto* linux_ui = views::LinuxUI::instance())
    return linux_ui->GetCursorBlinkInterval();
#elif defined(OS_WIN)
  const auto system_msec = ::GetCaretBlinkTime();
  if (system_msec != 0) {
    return (system_msec == INFINITE)
               ? base::TimeDelta()
               : base::TimeDelta::FromMilliseconds(system_msec);
  }
#endif
  return absl::nullopt;
}

#if BUILDFLAG(ENABLE_PRINTING)
// 如果不能使用提供的DEVICE_NAME打印，则返回FALSE。
// 在网络上找到的。我们需要检查这一点，因为铬不能。
// DEVICE_NAME有效性的健全性检查等将在无效名称上崩溃。
bool IsDeviceNameValid(const std::u16string& device_name) {
#if defined(OS_MAC)
  base::ScopedCFTypeRef<CFStringRef> new_printer_id(
      base::SysUTF16ToCFStringRef(device_name));
  PMPrinter new_printer = PMPrinterCreateFromPrinterID(new_printer_id.get());
  bool printer_exists = new_printer != nullptr;
  PMRelease(new_printer);
  return printer_exists;
#elif defined(OS_WIN)
  printing::ScopedPrinterHandle printer;
  return printer.OpenPrinterWithName(base::as_wcstr(device_name));
#else
  return true;
#endif
}

std::pair<std::string, std::u16string> GetDefaultPrinterAsync() {
#if defined(OS_WIN)
  // 这里需要阻止，因为Windows打印机驱动程序经常。
  // 不是线程安全的，必须在UI线程上访问。
  base::ThreadRestrictions::ScopedAllowIO allow_io;
#endif

  scoped_refptr<printing::PrintBackend> print_backend =
      printing::PrintBackend::CreateInstance(
          g_browser_process->GetApplicationLocale());
  std::string printer_name;
  printing::mojom::ResultCode code =
      print_backend->GetDefaultPrinterName(printer_name);

  // 如果此操作失败，我们不想退货，因为某些设备没有
  // 默认打印机。
  if (code != printing::mojom::ResultCode::kSuccess)
    LOG(ERROR) << "Failed to get default printer name";

  // 检查现有打印机并选择第一台打印机(如果存在)。
  if (printer_name.empty()) {
    printing::PrinterList printers;
    if (print_backend->EnumeratePrinters(&printers) !=
        printing::mojom::ResultCode::kSuccess)
      return std::make_pair("Failed to enumerate printers", std::u16string());
    if (!printers.empty())
      printer_name = printers.front().printer_name;
  }

  return std::make_pair(std::string(), base::UTF8ToUTF16(printer_name));
}

// 复制自。
// Chrome/browser/ui/webui/print_preview/local_printer_handler_default.cc:L36-L54。
scoped_refptr<base::TaskRunner> CreatePrinterHandlerTaskRunner() {
  // USER_VIRED，因为结果显示在打印预览对话框中。
#if !defined(OS_WIN)
  static constexpr base::TaskTraits kTraits = {
      base::MayBlock(), base::TaskPriority::USER_VISIBLE};
#endif

#if defined(USE_CUPS)
  // 杯子是线程安全的。
  return base::ThreadPool::CreateTaskRunner(kTraits);
#elif defined(OS_WIN)
  // Windows驱动程序可能不是线程安全的，需要在。
  // UI线程。
  return content::GetUIThreadTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE});
#else
  // 在不受支持的平台上保持保守。
  return base::ThreadPool::CreateSingleThreadTaskRunner(kTraits);
#endif
}
#endif

struct UserDataLink : public base::SupportsUserData::Data {
  explicit UserDataLink(base::WeakPtr<WebContents> contents)
      : web_contents(contents) {}

  base::WeakPtr<WebContents> web_contents;
};
const void* kElectronApiWebContentsKey = &kElectronApiWebContentsKey;

const char kRootName[] = "<root>";

struct FileSystem {
  FileSystem() = default;
  FileSystem(const std::string& type,
             const std::string& file_system_name,
             const std::string& root_url,
             const std::string& file_system_path)
      : type(type),
        file_system_name(file_system_name),
        root_url(root_url),
        file_system_path(file_system_path) {}

  std::string type;
  std::string file_system_name;
  std::string root_url;
  std::string file_system_path;
};

std::string RegisterFileSystem(content::WebContents* web_contents,
                               const base::FilePath& path) {
  auto* isolated_context = storage::IsolatedContext::GetInstance();
  std::string root_name(kRootName);
  storage::IsolatedContext::ScopedFSHandle file_system =
      isolated_context->RegisterFileSystemForPath(
          storage::kFileSystemTypeLocal, std::string(), path, &root_name);

  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  content::RenderViewHost* render_view_host = web_contents->GetRenderViewHost();
  int renderer_id = render_view_host->GetProcess()->GetID();
  policy->GrantReadFileSystem(renderer_id, file_system.id());
  policy->GrantWriteFileSystem(renderer_id, file_system.id());
  policy->GrantCreateFileForFileSystem(renderer_id, file_system.id());
  policy->GrantDeleteFromFileSystem(renderer_id, file_system.id());

  if (!policy->CanReadFile(renderer_id, path))
    policy->GrantReadFile(renderer_id, path);

  return file_system.id();
}

FileSystem CreateFileSystemStruct(content::WebContents* web_contents,
                                  const std::string& file_system_id,
                                  const std::string& file_system_path,
                                  const std::string& type) {
  const GURL origin = web_contents->GetURL().GetOrigin();
  std::string file_system_name =
      storage::GetIsolatedFileSystemName(origin, file_system_id);
  std::string root_url = storage::GetIsolatedFileSystemRootURIString(
      origin, file_system_id, kRootName);
  return FileSystem(type, file_system_name, root_url, file_system_path);
}

std::unique_ptr<base::DictionaryValue> CreateFileSystemValue(
    const FileSystem& file_system) {
  auto file_system_value = std::make_unique<base::DictionaryValue>();
  file_system_value->SetString("type", file_system.type);
  file_system_value->SetString("fileSystemName", file_system.file_system_name);
  file_system_value->SetString("rootURL", file_system.root_url);
  file_system_value->SetString("fileSystemPath", file_system.file_system_path);
  return file_system_value;
}

void WriteToFile(const base::FilePath& path, const std::string& content) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK(!path.empty());

  base::WriteFile(path, content.data(), content.size());
}

void AppendToFile(const base::FilePath& path, const std::string& content) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK(!path.empty());

  base::AppendToFile(path, content);
}

PrefService* GetPrefService(content::WebContents* web_contents) {
  auto* context = web_contents->GetBrowserContext();
  return static_cast<electron::ElectronBrowserContext*>(context)->prefs();
}

std::map<std::string, std::string> GetAddedFileSystemPaths(
    content::WebContents* web_contents) {
  auto* pref_service = GetPrefService(web_contents);
  const base::DictionaryValue* file_system_paths_value =
      pref_service->GetDictionary(prefs::kDevToolsFileSystemPaths);
  std::map<std::string, std::string> result;
  if (file_system_paths_value) {
    base::DictionaryValue::Iterator it(*file_system_paths_value);
    for (; !it.IsAtEnd(); it.Advance()) {
      std::string type =
          it.value().is_string() ? it.value().GetString() : std::string();
      result[it.key()] = type;
    }
  }
  return result;
}

bool IsDevToolsFileSystemAdded(content::WebContents* web_contents,
                               const std::string& file_system_path) {
  auto file_system_paths = GetAddedFileSystemPaths(web_contents);
  return file_system_paths.find(file_system_path) != file_system_paths.end();
}

}  // 命名空间。

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)

WebContents::Type GetTypeFromViewType(extensions::mojom::ViewType view_type) {
  switch (view_type) {
    case extensions::mojom::ViewType::kExtensionBackgroundPage:
      return WebContents::Type::kBackgroundPage;

    case extensions::mojom::ViewType::kAppWindow:
    case extensions::mojom::ViewType::kComponent:
    case extensions::mojom::ViewType::kExtensionDialog:
    case extensions::mojom::ViewType::kExtensionPopup:
    case extensions::mojom::ViewType::kBackgroundContents:
    case extensions::mojom::ViewType::kExtensionGuest:
    case extensions::mojom::ViewType::kTabContents:
    case extensions::mojom::ViewType::kInvalid:
      return WebContents::Type::kRemote;
  }
}

#endif

WebContents::WebContents(v8::Isolate* isolate,
                         content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      type_(Type::kRemote),
      id_(GetAllWebContents().Add(this)),
      devtools_file_system_indexer_(
          base::MakeRefCounted<DevToolsFileSystemIndexer>()),
      file_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}))
#if BUILDFLAG(ENABLE_PRINTING)
      ,
      print_task_runner_(CreatePrinterHandlerTaskRunner())
#endif
{
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // 扩展主机创建的WebContents将设置有效的ViewType。
  extensions::mojom::ViewType view_type = extensions::GetViewType(web_contents);
  if (view_type != extensions::mojom::ViewType::kInvalid) {
    InitWithExtensionView(isolate, web_contents, view_type);
  }

  extensions::ElectronExtensionWebContentsObserver::CreateForWebContents(
      web_contents);
  script_executor_ = std::make_unique<extensions::ScriptExecutor>(web_contents);
#endif

  auto session = Session::CreateFrom(isolate, GetBrowserContext());
  session_.Reset(isolate, session.ToV8());

  web_contents->SetUserAgentOverride(blink::UserAgentOverride::UserAgentOnly(
                                         GetBrowserContext()->GetUserAgent()),
                                     false);
  web_contents->SetUserData(kElectronApiWebContentsKey,
                            std::make_unique<UserDataLink>(GetWeakPtr()));
  InitZoomController(web_contents, gin::Dictionary::CreateEmpty(isolate));
}

WebContents::WebContents(v8::Isolate* isolate,
                         std::unique_ptr<content::WebContents> web_contents,
                         Type type)
    : content::WebContentsObserver(web_contents.get()),
      type_(type),
      id_(GetAllWebContents().Add(this)),
      devtools_file_system_indexer_(
          base::MakeRefCounted<DevToolsFileSystemIndexer>()),
      file_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}))
#if BUILDFLAG(ENABLE_PRINTING)
      ,
      print_task_runner_(CreatePrinterHandlerTaskRunner())
#endif
{
  DCHECK(type != Type::kRemote)
      << "Can't take ownership of a remote WebContents";
  auto session = Session::CreateFrom(isolate, GetBrowserContext());
  session_.Reset(isolate, session.ToV8());
  InitWithSessionAndOptions(isolate, std::move(web_contents), session,
                            gin::Dictionary::CreateEmpty(isolate));
}

WebContents::WebContents(v8::Isolate* isolate,
                         const gin_helper::Dictionary& options)
    : id_(GetAllWebContents().Add(this)),
      devtools_file_system_indexer_(
          base::MakeRefCounted<DevToolsFileSystemIndexer>()),
      file_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}))
#if BUILDFLAG(ENABLE_PRINTING)
      ,
      print_task_runner_(CreatePrinterHandlerTaskRunner())
#endif
{
  // 读取选项。
  options.Get("backgroundThrottling", &background_throttling_);

  // 获取类型。
  options.Get("type", &type_);

#if BUILDFLAG(ENABLE_OSR)
  bool b = false;
  if (options.Get(options::kOffscreen, &b) && b)
    type_ = Type::kOffScreen;
#endif

  // 较早的初始化嵌入器。
  options.Get("embedder", &embedder_);

  // 是否启用DevTools。
  options.Get("devTools", &enable_devtools_);

  // BrowserView最初没有附加到窗口，因此它们应该启动。
  // 隐藏起来。这对于合成器回收也很重要。请参见：
  // Https://github.com/electron/electron/pull/21372。
  bool initially_shown = type_ != Type::kBrowserView;
  options.Get(options::kShow, &initially_shown);

  // 获取会话。
  std::string partition;
  gin::Handle<api::Session> session;
  if (options.Get("session", &session) && !session.IsEmpty()) {
  } else if (options.Get("partition", &partition)) {
    session = Session::FromPartition(isolate, partition);
  } else {
    // 如果未指定，请使用默认会话。
    session = Session::FromPartition(isolate, "");
  }
  session_.Reset(isolate, session.ToV8());

  std::unique_ptr<content::WebContents> web_contents;
  if (IsGuest()) {
    scoped_refptr<content::SiteInstance> site_instance =
        content::SiteInstance::CreateForURL(session->browser_context(),
                                            GURL("chrome-guest:// 假主持人“))；
    content::WebContents::CreateParams params(session->browser_context(),
                                              site_instance);
    guest_delegate_ =
        std::make_unique<WebViewGuestDelegate>(embedder_->web_contents(), this);
    params.guest_delegate = guest_delegate_.get();

#if BUILDFLAG(ENABLE_OSR)
    if (embedder_ && embedder_->IsOffScreen()) {
      auto* view = new OffScreenWebContentsView(
          false,
          base::BindRepeating(&WebContents::OnPaint, base::Unretained(this)));
      params.view = view;
      params.delegate_view = view;

      web_contents = content::WebContents::Create(params);
      view->SetWebContents(web_contents.get());
    } else {
#endif
      web_contents = content::WebContents::Create(params);
#if BUILDFLAG(ENABLE_OSR)
    }
  } else if (IsOffScreen()) {
    bool transparent = false;
    options.Get(options::kTransparent, &transparent);

    content::WebContents::CreateParams params(session->browser_context());
    auto* view = new OffScreenWebContentsView(
        transparent,
        base::BindRepeating(&WebContents::OnPaint, base::Unretained(this)));
    params.view = view;
    params.delegate_view = view;

    web_contents = content::WebContents::Create(params);
    view->SetWebContents(web_contents.get());
#endif
  } else {
    content::WebContents::CreateParams params(session->browser_context());
    params.initially_hidden = !initially_shown;
    web_contents = content::WebContents::Create(params);
  }

  InitWithSessionAndOptions(isolate, std::move(web_contents), session, options);
}

void WebContents::InitZoomController(content::WebContents* web_contents,
                                     const gin_helper::Dictionary& options) {
  WebContentsZoomController::CreateForWebContents(web_contents);
  zoom_controller_ = WebContentsZoomController::FromWebContents(web_contents);
  double zoom_factor;
  if (options.Get(options::kZoomFactor, &zoom_factor))
    zoom_controller_->SetDefaultZoomFactor(zoom_factor);
}

void WebContents::InitWithSessionAndOptions(
    v8::Isolate* isolate,
    std::unique_ptr<content::WebContents> owned_web_contents,
    gin::Handle<api::Session> session,
    const gin_helper::Dictionary& options) {
  Observe(owned_web_contents.get());
  InitWithWebContents(std::move(owned_web_contents), session->browser_context(),
                      IsGuest());

  inspectable_web_contents_->GetView()->SetDelegate(this);

  auto* prefs = web_contents()->GetMutableRendererPrefs();

  // 从操作系统和浏览器进程收集首选语言。接受语言(_L)。
  // 影响HTTP标题、导航器、语言和CJK回退字体选择。
  // 
  // 请注意，设置为浏览器进程的应用程序区域设置可能为。
  // 与设置到首选项列表的不同。
  // (例如，用--lang覆盖)。
  std::string accept_languages =
      g_browser_process->GetApplicationLocale() + ",";
  for (auto const& language : electron::GetPreferredLanguages()) {
    if (language == g_browser_process->GetApplicationLocale())
      continue;
    accept_languages += language + ",";
  }
  accept_languages.pop_back();
  prefs->accept_languages = accept_languages;

#if defined(OS_LINUX) || defined(OS_WIN)
  // 更新字体设置。
  static const gfx::FontRenderParams params(
      gfx::GetFontRenderParams(gfx::FontRenderParamsQuery(), nullptr));
  prefs->should_antialias_text = params.antialiasing;
  prefs->use_subpixel_positioning = params.subpixel_positioning;
  prefs->hinting = params.hinting;
  prefs->use_autohinter = params.autohinter;
  prefs->use_bitmaps = params.use_bitmaps;
  prefs->subpixel_rendering = params.subpixel_rendering;
#endif

  // 遵守系统的光标闪烁速率设置。
  if (auto interval = GetCursorBlinkInterval())
    prefs->caret_blink_interval = *interval;

  // 用C++保存首选项。
  // 如果已经有WebContentsPreferences对象，我们将其创建为。
  // 使用webContents.setWindowOpenHandler路径，所以不要覆盖它。
  if (!WebContentsPreferences::From(web_contents())) {
    new WebContentsPreferences(web_contents(), options);
  }
  // 触发Webkit首选项的重新计算。
  web_contents()->NotifyPreferencesChanged();

  WebContentsPermissionHelper::CreateForWebContents(web_contents());
  InitZoomController(web_contents(), options);
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions::ElectronExtensionWebContentsObserver::CreateForWebContents(
      web_contents());
  script_executor_ =
      std::make_unique<extensions::ScriptExecutor>(web_contents());
#endif

  AutofillDriverFactory::CreateForWebContents(web_contents());

  web_contents()->SetUserAgentOverride(blink::UserAgentOverride::UserAgentOnly(
                                           GetBrowserContext()->GetUserAgent()),
                                       false);

  if (IsGuest()) {
    NativeWindow* owner_window = nullptr;
    if (embedder_) {
      // 新WebContents的Owner_Window是嵌入器的Owner_Window。
      auto* relay =
          NativeWindowRelay::FromWebContents(embedder_->web_contents());
      if (relay)
        owner_window = relay->GetNativeWindow();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);
  }

  web_contents()->SetUserData(kElectronApiWebContentsKey,
                              std::make_unique<UserDataLink>(GetWeakPtr()));
}

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
void WebContents::InitWithExtensionView(v8::Isolate* isolate,
                                        content::WebContents* web_contents,
                                        extensions::mojom::ViewType view_type) {
  // 在调用`Init`之前必须重新分配类型。
  type_ = GetTypeFromViewType(view_type);
  if (type_ == Type::kRemote)
    return;
  if (type_ == Type::kBackgroundPage)
    // 非背景页WebContents由其他类保留。我们需要。
    // 锁定此处以防止背景页WebContents被GC。
    // 后台页面API：：WebContents将一直存在到基础。
    // 内容：：WebContents被销毁。
    Pin(isolate);

  // 允许切换背景页的DevTools
  Observe(web_contents);
  InitWithWebContents(std::unique_ptr<content::WebContents>(web_contents),
                      GetBrowserContext(), IsGuest());
  inspectable_web_contents_->GetView()->SetDelegate(this);
}
#endif

void WebContents::InitWithWebContents(
    std::unique_ptr<content::WebContents> web_contents,
    ElectronBrowserContext* browser_context,
    bool is_guest) {
  browser_context_ = browser_context;
  web_contents->SetDelegate(this);

#if BUILDFLAG(ENABLE_PRINTING)
  PrintPreviewMessageHandler::CreateForWebContents(web_contents.get());
  PrintViewManagerElectron::CreateForWebContents(web_contents.get());
  printing::CreateCompositeClientIfNeeded(web_contents.get(),
                                          browser_context->GetUserAgent());
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  pdf::PDFWebContentsHelper::CreateForWebContentsWithClient(
      web_contents.get(),
      std::make_unique<ElectronPDFWebContentsHelperClient>());
#endif

  // 确定WebContents是否在屏幕外。
  auto* web_preferences = WebContentsPreferences::From(web_contents.get());
  offscreen_ = web_preferences && web_preferences->IsOffscreen();

  // 创建InspectableWebContents。
  inspectable_web_contents_ = std::make_unique<InspectableWebContents>(
      std::move(web_contents), browser_context->prefs(), is_guest);
  inspectable_web_contents_->SetDelegate(this);
}

WebContents::~WebContents() {
  // 清除已授予权限的对象，以便在。
  // WebContents：：RenderFrameDeleted作为WebContents的结果被调用。
  // 销毁它不会尝试清除GRANT_DEVICES_。
  // 在一个被破坏的物体上。
  granted_devices_.clear();

  if (!inspectable_web_contents_) {
    WebContentsDestroyed();
    return;
  }

  inspectable_web_contents_->GetView()->SetDelegate(nullptr);

  // 此事件仅供内部使用，当WebContents为。
  // 被摧毁了。
  Emit("will-destroy");

  // 对于基于OOPIF的访客视图，WebContents由嵌入器发布。
  // 帧，我们需要清除对内存的引用。
  bool not_owned_by_this = IsGuest() && attached_;
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // 后台页面归Extensions：：ExtensionHost所有。
  if (type_ == Type::kBackgroundPage)
    not_owned_by_this = true;
#endif
  if (not_owned_by_this) {
    inspectable_web_contents_->ReleaseWebContents();
    WebContentsDestroyed();
  }

  // InspectableWebContents将自动销毁。
}

void WebContents::DeleteThisIfAlive() {
  // 可能已经调用了FirstWeakCallback，但。
  // Second WeakCallback没有，在本例中是垃圾回收。
  // WebContents已启动，我们不应|删除此|。
  // 调用|GetWrapper|可以检测到这种角落大小写。
  auto* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper))
    return;
  delete this;
}

void WebContents::Destroy() {
  // 如果可能，应该异步销毁Content：：WebContents。
  // 因为用户可以选择在WebContents事件期间销毁WebContents。
  if (Browser::Get()->is_shutting_down() || IsGuest() ||
      type_ == Type::kBrowserView) {
    DeleteThisIfAlive();
  } else {
    base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                   base::BindOnce(
                       [](base::WeakPtr<WebContents> contents) {
                         if (contents)
                           contents->DeleteThisIfAlive();
                       },
                       GetWeakPtr()));
  }
}

bool WebContents::DidAddMessageToConsole(
    content::WebContents* source,
    blink::mojom::ConsoleMessageLevel level,
    const std::u16string& message,
    int32_t line_no,
    const std::u16string& source_id) {
  return Emit("console-message", static_cast<int32_t>(level), message, line_no,
              source_id);
}

void WebContents::OnCreateWindow(
    const GURL& target_url,
    const content::Referrer& referrer,
    const std::string& frame_name,
    WindowOpenDisposition disposition,
    const std::string& features,
    const scoped_refptr<network::ResourceRequestBody>& body) {
  Emit("-new-window", target_url, frame_name, disposition, features, referrer,
       body);
}

void WebContents::WebContentsCreatedWithFullParams(
    content::WebContents* source_contents,
    int opener_render_process_id,
    int opener_render_frame_id,
    const content::mojom::CreateNewWindowParams& params,
    content::WebContents* new_contents) {
  ChildWebContentsTracker::CreateForWebContents(new_contents);
  auto* tracker = ChildWebContentsTracker::FromWebContents(new_contents);
  tracker->url = params.target_url;
  tracker->frame_name = params.frame_name;
  tracker->referrer = params.referrer.To<content::Referrer>();
  tracker->raw_features = params.raw_features;
  tracker->body = params.body;

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  gin_helper::Dictionary dict;
  gin::ConvertFromV8(isolate, pending_child_web_preferences_.Get(isolate),
                     &dict);
  pending_child_web_preferences_.Reset();

  // 将通过`setWindowOpenHandler`传入的首选项与。
  // 刚刚为子窗口创建的Content：：WebContents。这些
  // 首选项将由RenderWidgetHost通过调用。
  // 委托的OverrideWebkitPrefs。
  new WebContentsPreferences(new_contents, dict);
}

bool WebContents::IsWebContentsCreationOverridden(
    content::SiteInstance* source_site_instance,
    content::mojom::WindowContainerType window_container_type,
    const GURL& opener_url,
    const content::mojom::CreateNewWindowParams& params) {
  bool default_prevented = Emit(
      "-will-add-new-contents", params.target_url, params.frame_name,
      params.raw_features, params.disposition, *params.referrer, params.body);
  // 如果应用程序阻止默认设置，请重定向到CreateCustomWebContents。
  // 它总是返回nullptr，这将导致窗口打开。
  // 已阻止(window.open()将在渲染器中返回NULL)。
  return default_prevented;
}

void WebContents::SetNextChildWebPreferences(
    const gin_helper::Dictionary preferences) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  // 在Chrome调用WebContentsCreatedWithFullParams时存储这些首选项。
  // 包含新的子项内容。
  pending_child_web_preferences_.Reset(isolate, preferences.GetHandle());
}

content::WebContents* WebContents::CreateCustomWebContents(
    content::RenderFrameHost* opener,
    content::SiteInstance* source_site_instance,
    bool is_new_browsing_instance,
    const GURL& opener_url,
    const std::string& frame_name,
    const GURL& target_url,
    const content::StoragePartitionId& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  return nullptr;
}

void WebContents::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture,
    bool* was_blocked) {
  auto* tracker = ChildWebContentsTracker::FromWebContents(new_contents.get());
  DCHECK(tracker);

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();

  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  auto api_web_contents =
      CreateAndTake(isolate, std::move(new_contents), Type::kBrowserWindow);

  // 我们在此处将RenderFrameCreated称为空的“About：Blank”
  // 渲染帧已创建。如果窗口再也不能导航。
  // RenderFrameCreated不会被调用，并且某些首选项，如。
  // 不会应用“kBackround Color”。
  auto* frame = api_web_contents->MainFrame();
  if (frame) {
    api_web_contents->HandleNewRenderFrame(frame);
  }

  if (Emit("-add-new-contents", api_web_contents, disposition, user_gesture,
           initial_rect.x(), initial_rect.y(), initial_rect.width(),
           initial_rect.height(), tracker->url, tracker->frame_name,
           tracker->referrer, tracker->raw_features, tracker->body)) {
    api_web_contents->Destroy();
  }
}

content::WebContents* WebContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  auto weak_this = GetWeakPtr();
  if (params.disposition != WindowOpenDisposition::CURRENT_TAB) {
    Emit("-new-window", params.url, "", params.disposition, "", params.referrer,
         params.post_data);
    return nullptr;
  }

  if (!weak_this || !web_contents())
    return nullptr;

  content::NavigationController::LoadURLParams load_url_params(params.url);
  load_url_params.referrer = params.referrer;
  load_url_params.transition_type = params.transition;
  load_url_params.extra_headers = params.extra_headers;
  load_url_params.should_replace_current_entry =
      params.should_replace_current_entry;
  load_url_params.is_renderer_initiated = params.is_renderer_initiated;
  load_url_params.started_from_context_menu = params.started_from_context_menu;
  load_url_params.initiator_origin = params.initiator_origin;
  load_url_params.source_site_instance = params.source_site_instance;
  load_url_params.frame_tree_node_id = params.frame_tree_node_id;
  load_url_params.redirect_chain = params.redirect_chain;
  load_url_params.has_user_gesture = params.user_gesture;
  load_url_params.blob_url_loader_factory = params.blob_url_loader_factory;
  load_url_params.href_translate = params.href_translate;
  load_url_params.reload_type = params.reload_type;

  if (params.post_data) {
    load_url_params.load_type =
        content::NavigationController::LOAD_TYPE_HTTP_POST;
    load_url_params.post_data = params.post_data;
  }

  source->GetController().LoadURLWithParams(load_url_params);
  return source;
}

void WebContents::BeforeUnloadFired(content::WebContents* tab,
                                    bool proceed,
                                    bool* proceed_to_fire_unload) {
  if (type_ == Type::kBrowserWindow || type_ == Type::kOffScreen ||
      type_ == Type::kBrowserView)
    *proceed_to_fire_unload = proceed;
  else
    *proceed_to_fire_unload = true;
  // 请注意，Chromium在导航时不会发出此消息。
  Emit("before-unload-fired", proceed);
}

void WebContents::SetContentsBounds(content::WebContents* source,
                                    const gfx::Rect& rect) {
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnSetContentBounds(rect);
}

void WebContents::CloseContents(content::WebContents* source) {
  Emit("close");

  auto* autofill_driver_factory =
      AutofillDriverFactory::FromWebContents(web_contents());
  if (autofill_driver_factory) {
    autofill_driver_factory->CloseAllPopups();
  }

  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnCloseContents();
}

void WebContents::ActivateContents(content::WebContents* source) {
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnActivateContents();
}

void WebContents::UpdateTargetURL(content::WebContents* source,
                                  const GURL& url) {
  Emit("update-target-url", url);
}

bool WebContents::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (type_ == Type::kWebView && embedder_) {
    // 将未处理的键盘事件发送回嵌入器。
    return embedder_->HandleKeyboardEvent(source, event);
  } else {
    return PlatformHandleKeyboardEvent(source, event);
  }
}

#if !defined(OS_MAC)
// 注意：此函数的MacOS版本位于。
// Electronics_API_web_Contents_mac.mm，因为它需要调用Objective-C。
// 密码。
bool WebContents::PlatformHandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  // 退出退出选项卡式全屏模式。
  if (event.windows_key_code == ui::VKEY_ESCAPE && is_html_fullscreen()) {
    ExitFullscreenModeForTab(source);
    return true;
  }

  // 检查webContents是否具有首选项并忽略快捷方式。
  auto* web_preferences = WebContentsPreferences::From(source);
  if (web_preferences && web_preferences->ShouldIgnoreMenuShortcuts())
    return false;

  // 让NativeWindow处理其他部分。
  if (owner_window()) {
    owner_window()->HandleKeyboardEvent(source, event);
    return true;
  }

  return false;
}
#endif

content::KeyboardEventProcessingResult WebContents::PreHandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (event.GetType() == blink::WebInputEvent::Type::kRawKeyDown ||
      event.GetType() == blink::WebInputEvent::Type::kKeyUp) {
    bool prevent_default = Emit("before-input-event", event);
    if (prevent_default) {
      return content::KeyboardEventProcessingResult::HANDLED;
    }
  }

  return content::KeyboardEventProcessingResult::NOT_HANDLED;
}

void WebContents::ContentsZoomChange(bool zoom_in) {
  Emit("zoom-changed", zoom_in ? "in" : "out");
}

void WebContents::EnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame,
    const blink::mojom::FullscreenOptions& options) {
  auto* source = content::WebContents::FromRenderFrameHost(requesting_frame);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(source);
  auto callback =
      base::BindRepeating(&WebContents::OnEnterFullscreenModeForTab,
                          base::Unretained(this), requesting_frame, options);
  permission_helper->RequestFullscreenPermission(callback);
}

void WebContents::OnEnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame,
    const blink::mojom::FullscreenOptions& options,
    bool allowed) {
  if (!allowed || !owner_window_)
    return;

  auto* source = content::WebContents::FromRenderFrameHost(requesting_frame);
  if (IsFullscreenForTabOrPending(source)) {
    DCHECK_EQ(fullscreen_frame_, source->GetFocusedFrame());
    return;
  }

  SetHtmlApiFullscreen(true);

  if (native_fullscreen_) {
    // 显式触发视图大小调整，因为如果
    // 浏览器也是全屏显示的。
    source->GetRenderViewHost()->GetWidget()->SynchronizeVisualProperties();
  }
}

void WebContents::ExitFullscreenModeForTab(content::WebContents* source) {
  if (!owner_window_)
    return;

  SetHtmlApiFullscreen(false);

  if (native_fullscreen_) {
    // 显式触发视图大小调整，因为如果。
    // 浏览器也是全屏显示的。Chrome通过以下方式间接地做到了这一点。
    // `chrome/browser/ui/exclusive_access/fullscreen_controller.cc`.。
    source->GetRenderViewHost()->GetWidget()->SynchronizeVisualProperties();
  }
}

void WebContents::RendererUnresponsive(
    content::WebContents* source,
    content::RenderWidgetHost* render_widget_host,
    base::RepeatingClosure hang_monitor_restarter) {
  Emit("unresponsive");
}

void WebContents::RendererResponsive(
    content::WebContents* source,
    content::RenderWidgetHost* render_widget_host) {
  Emit("responsive");
}

bool WebContents::HandleContextMenu(content::RenderFrameHost* render_frame_host,
                                    const content::ContextMenuParams& params) {
  Emit("context-menu", std::make_pair(params, render_frame_host));

  return true;
}

bool WebContents::OnGoToEntryOffset(int offset) {
  GoToOffset(offset);
  return false;
}

void WebContents::FindReply(content::WebContents* web_contents,
                            int request_id,
                            int number_of_matches,
                            const gfx::Rect& selection_rect,
                            int active_match_ordinal,
                            bool final_update) {
  if (!final_update)
    return;

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  gin_helper::Dictionary result = gin::Dictionary::CreateEmpty(isolate);
  result.Set("requestId", request_id);
  result.Set("matches", number_of_matches);
  result.Set("selectionArea", selection_rect);
  result.Set("activeMatchOrdinal", active_match_ordinal);
  result.Set("finalUpdate", final_update);  // 2.0之后不推荐使用。
  Emit("found-in-page", result.GetHandle());
}

bool WebContents::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    blink::mojom::MediaStreamType type) {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  return permission_helper->CheckMediaAccessPermission(security_origin, type);
}

void WebContents::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback) {
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestMediaAccessPermission(request, std::move(callback));
}

void WebContents::RequestToLockMouse(content::WebContents* web_contents,
                                     bool user_gesture,
                                     bool last_unlocked_by_target) {
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestPointerLockPermission(user_gesture);
}

content::JavaScriptDialogManager* WebContents::GetJavaScriptDialogManager(
    content::WebContents* source) {
  if (!dialog_manager_)
    dialog_manager_ = std::make_unique<ElectronJavaScriptDialogManager>();

  return dialog_manager_.get();
}

void WebContents::OnAudioStateChanged(bool audible) {
  Emit("-audio-state-changed", audible);
}

void WebContents::BeforeUnloadFired(bool proceed,
                                    const base::TimeTicks& proceed_time) {
  // 什么都不做，我们重写此方法只是为了避免编译错误，因为。
  // 有两个名为BeforeUnloadFired的虚拟函数。
}

void WebContents::HandleNewRenderFrame(
    content::RenderFrameHost* render_frame_host) {
  auto* rwhv = render_frame_host->GetView();
  if (!rwhv)
    return;

  // 设置RenderWidgetHostView的背景色。
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  if (web_preferences) {
    absl::optional<SkColor> color = web_preferences->GetBackgroundColor();
    web_contents()->SetPageBaseBackgroundColor(color);
    rwhv->SetBackgroundColor(color.value_or(SK_ColorWHITE));
  }

  if (!background_throttling_)
    render_frame_host->GetRenderViewHost()->SetSchedulerThrottling(false);

  auto* rwh_impl =
      static_cast<content::RenderWidgetHostImpl*>(rwhv->GetRenderWidgetHost());
  if (rwh_impl)
    rwh_impl->disable_hidden_ = !background_throttling_;

  auto* web_frame = WebFrameMain::FromRenderFrameHost(render_frame_host);
  if (web_frame)
    web_frame->Connect();
}

void WebContents::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  HandleNewRenderFrame(render_frame_host);

  // 为推测帧调用RenderFrameCreated，这些帧可能不是。
  // 用于某些跨地域航行。正在调用。
  // RenderFrameHost：：GetLifecycleState当前在调用时崩溃。
  // 投机性的帧，所以我们现在需要把它过滤掉。检查。
  // Https://crbug.com/1183639，了解何时可以删除此文件的详细信息。
  auto* rfh_impl =
      static_cast<content::RenderFrameHostImpl*>(render_frame_host);
  if (rfh_impl->lifecycle_state() ==
      content::RenderFrameHostImpl::LifecycleStateImpl::kSpeculative) {
    return;
  }

  content::RenderFrameHost::LifecycleState lifecycle_state =
      render_frame_host->GetLifecycleState();
  if (lifecycle_state == content::RenderFrameHost::LifecycleState::kActive) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    gin_helper::Dictionary details =
        gin_helper::Dictionary::CreateEmpty(isolate);
    details.SetGetter("frame", render_frame_host);
    Emit("frame-created", details);
  }
}

void WebContents::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  // 在以下情况下可以删除RenderFrameHost：
  // -移除WebContents并释放其包含的框架。
  // -从DOM中删除&lt;iframe&gt;。
  // -跨源导航在单独的进程中创建新的RFH，该进程。
  // 由Content：：RenderFrameHostManager交换。
  // 

  // 清除已被授予权限的对象
  if (!granted_devices_.empty()) {
    granted_devices_.erase(render_frame_host->GetFrameTreeNodeId());
  }

  // WebFrameMain：：FromRenderFrameHost(RFH)将使用RFH的FrameTreeNode ID。
  // 若要查找WebFrameMain的现有实例，请执行以下操作。在跨地域旅行期间。
  // 导航时，删除的RFH将是换出的旧主机。在……里面。
  // 在这种特殊情况下，我们还需要确保WebFrameMain的内部RFH。
  // 在将其标记为已释放之前进行匹配。
  auto* web_frame = WebFrameMain::FromRenderFrameHost(render_frame_host);
  if (web_frame && web_frame->render_frame_host() == render_frame_host)
    web_frame->MarkRenderFrameDisposed();
}

void WebContents::RenderFrameHostChanged(content::RenderFrameHost* old_host,
                                         content::RenderFrameHost* new_host) {
  // 在跨域导航时，FrameTreeNode会换出它的RFH。
  // 如果存在WebFrameMain实例，则需要具有其RFH。
  // 也交换了。
  // 
  // |old_host|可以是nullptr，所以我们使用|new_host|来查找。
  // WebFrameMain实例。
  auto* web_frame =
      WebFrameMain::FromFrameTreeNodeId(new_host->GetFrameTreeNodeId());
  if (web_frame) {
    web_frame->UpdateRenderFrameHost(new_host);
  }
}

void WebContents::FrameDeleted(int frame_tree_node_id) {
  auto* web_frame = WebFrameMain::FromFrameTreeNodeId(frame_tree_node_id);
  if (web_frame)
    web_frame->Destroyed();
}

void WebContents::RenderViewDeleted(content::RenderViewHost* render_view_host) {
  // 此事件对于跟踪与以下各项相关的任何状态都是必需的。
  // 中间渲染视图主机(也称为推测性渲染视图主机)。目前。
  // 由object-registry.js用于引用远程对象的计数。
  Emit("render-view-deleted", render_view_host->GetProcess()->GetID());

  if (web_contents()->GetRenderViewHost() == render_view_host) {
    // 当已删除的RVH为当前RVH时，表示。
    // 网站内容正在关闭。这是由这个事件传达的。
    // 当前由Guest-Window-Manager.ts跟踪以销毁。
    // 浏览器窗口。
    Emit("current-render-view-deleted",
         render_view_host->GetProcess()->GetID());
  }
}

void WebContents::RenderProcessGone(base::TerminationStatus status) {
  auto weak_this = GetWeakPtr();
  Emit("crashed", status == base::TERMINATION_STATUS_PROCESS_WAS_KILLED);

  // 用户可能会在Crash事件中销毁WebContents。
  if (!weak_this || !web_contents())
    return;

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  gin_helper::Dictionary details = gin_helper::Dictionary::CreateEmpty(isolate);
  details.Set("reason", status);
  details.Set("exitCode", web_contents()->GetCrashedErrorCode());
  Emit("render-process-gone", details);
}

void WebContents::PluginCrashed(const base::FilePath& plugin_path,
                                base::ProcessId plugin_pid) {
#if BUILDFLAG(ENABLE_PLUGINS)
  content::WebPluginInfo info;
  auto* plugin_service = content::PluginService::GetInstance();
  plugin_service->GetPluginInfoByPath(plugin_path, &info);
  Emit("plugin-crashed", info.name, info.version);
#endif  // BUILDFLAG(ENABLE_Plugin)
}

void WebContents::MediaStartedPlaying(const MediaPlayerInfo& video_type,
                                      const content::MediaPlayerId& id) {
  Emit("media-started-playing");
}

void WebContents::MediaStoppedPlaying(
    const MediaPlayerInfo& video_type,
    const content::MediaPlayerId& id,
    content::WebContentsObserver::MediaStoppedReason reason) {
  Emit("media-paused");
}

void WebContents::DidChangeThemeColor() {
  auto theme_color = web_contents()->GetThemeColor();
  if (theme_color) {
    Emit("did-change-theme-color", electron::ToRGBHex(theme_color.value()));
  } else {
    Emit("did-change-theme-color", nullptr);
  }
}

void WebContents::DidAcquireFullscreen(content::RenderFrameHost* rfh) {
  set_fullscreen_frame(rfh);
}

void WebContents::DOMContentLoaded(
    content::RenderFrameHost* render_frame_host) {
  auto* web_frame = WebFrameMain::FromRenderFrameHost(render_frame_host);
  if (web_frame)
    web_frame->DOMContentLoaded();

  if (!render_frame_host->GetParent())
    Emit("dom-ready");
}

void WebContents::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                const GURL& validated_url) {
  bool is_main_frame = !render_frame_host->GetParent();
  int frame_process_id = render_frame_host->GetProcess()->GetID();
  int frame_routing_id = render_frame_host->GetRoutingID();
  auto weak_this = GetWeakPtr();
  Emit("did-frame-finish-load", is_main_frame, frame_process_id,
       frame_routing_id);

  // ⚠️警告！⚠️。
  // Emit()触发JS，JS可以在|this|上调用Destroy()。这是不安全的。
  // 假设|this|指向此时的有效内存。
  if (is_main_frame && weak_this && web_contents())
    Emit("did-finish-load");
}

void WebContents::DidFailLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& url,
                              int error_code) {
  bool is_main_frame = !render_frame_host->GetParent();
  int frame_process_id = render_frame_host->GetProcess()->GetID();
  int frame_routing_id = render_frame_host->GetRoutingID();
  Emit("did-fail-load", error_code, "", url, is_main_frame, frame_process_id,
       frame_routing_id);
}

void WebContents::DidStartLoading() {
  Emit("did-start-loading");
}

void WebContents::DidStopLoading() {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  if (web_preferences && web_preferences->ShouldUsePreferredSizeMode())
    web_contents()->GetRenderViewHost()->EnablePreferredSizeMode();

  Emit("did-stop-loading");
}

bool WebContents::EmitNavigationEvent(
    const std::string& event,
    content::NavigationHandle* navigation_handle) {
  bool is_main_frame = navigation_handle->IsInMainFrame();
  int frame_tree_node_id = navigation_handle->GetFrameTreeNodeId();
  content::FrameTreeNode* frame_tree_node =
      content::FrameTreeNode::GloballyFindByID(frame_tree_node_id);
  content::RenderFrameHostManager* render_manager =
      frame_tree_node->render_manager();
  content::RenderFrameHost* frame_host = nullptr;
  if (render_manager) {
    frame_host = render_manager->speculative_frame_host();
    if (!frame_host)
      frame_host = render_manager->current_frame_host();
  }
  int frame_process_id = -1, frame_routing_id = -1;
  if (frame_host) {
    frame_process_id = frame_host->GetProcess()->GetID();
    frame_routing_id = frame_host->GetRoutingID();
  }
  bool is_same_document = navigation_handle->IsSameDocument();
  auto url = navigation_handle->GetURL();
  return Emit(event, url, is_same_document, is_main_frame, frame_process_id,
              frame_routing_id);
}

void WebContents::Message(bool internal,
                          const std::string& channel,
                          blink::CloneableMessage arguments,
                          content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::Message", "channel", channel);
  // WebContents.emit(‘-ipc-message’，new event()，Internal，channel，
  // 论据)；
  EmitWithSender("-ipc-message", render_frame_host,
                 electron::mojom::ElectronBrowser::InvokeCallback(), internal,
                 channel, std::move(arguments));
}

void WebContents::Invoke(
    bool internal,
    const std::string& channel,
    blink::CloneableMessage arguments,
    electron::mojom::ElectronBrowser::InvokeCallback callback,
    content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::Invoke", "channel", channel);
  // WebContents.emit(‘-IPC-Invoke’，new event()，Internal，Channel，Arguments)；
  EmitWithSender("-ipc-invoke", render_frame_host, std::move(callback),
                 internal, channel, std::move(arguments));
}

void WebContents::OnFirstNonEmptyLayout(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == web_contents()->GetMainFrame()) {
    Emit("ready-to-show");
  }
}

void WebContents::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message,
    content::RenderFrameHost* render_frame_host) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto wrapped_ports =
      MessagePort::EntanglePorts(isolate, std::move(message.ports));
  v8::Local<v8::Value> message_value =
      electron::DeserializeV8Value(isolate, message);
  EmitWithSender("-ipc-ports", render_frame_host,
                 electron::mojom::ElectronBrowser::InvokeCallback(), false,
                 channel, message_value, std::move(wrapped_ports));
}

void WebContents::MessageSync(
    bool internal,
    const std::string& channel,
    blink::CloneableMessage arguments,
    electron::mojom::ElectronBrowser::MessageSyncCallback callback,
    content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::MessageSync", "channel", channel);
  // WebContents.emit(‘-ipc-message-sync’，新事件(发送者，消息)，内部，
  // 通道、参数)；
  EmitWithSender("-ipc-message-sync", render_frame_host, std::move(callback),
                 internal, channel, std::move(arguments));
}

void WebContents::MessageTo(int32_t web_contents_id,
                            const std::string& channel,
                            blink::CloneableMessage arguments) {
  TRACE_EVENT1("electron", "WebContents::MessageTo", "channel", channel);
  auto* target_web_contents = FromID(web_contents_id);

  if (target_web_contents) {
    content::RenderFrameHost* frame = target_web_contents->MainFrame();
    DCHECK(frame);

    v8::HandleScope handle_scope(JavascriptEnvironment::GetIsolate());
    gin::Handle<WebFrameMain> web_frame_main =
        WebFrameMain::From(JavascriptEnvironment::GetIsolate(), frame);

    if (!web_frame_main->CheckRenderFrame())
      return;

    int32_t sender_id = ID();
    web_frame_main->GetRendererApi()->Message(false /* 内部。*/, channel,
                                              std::move(arguments), sender_id);
  }
}

void WebContents::MessageHost(const std::string& channel,
                              blink::CloneableMessage arguments,
                              content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::MessageHost", "channel", channel);
  // WebContents.emit(‘ipc-message-host’，new event()，channel，args)；
  EmitWithSender("ipc-message-host", render_frame_host,
                 electron::mojom::ElectronBrowser::InvokeCallback(), channel,
                 std::move(arguments));
}

void WebContents::UpdateDraggableRegions(
    std::vector<mojom::DraggableRegionPtr> regions) {
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnDraggableRegionsUpdated(regions);
}

void WebContents::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  EmitNavigationEvent("did-start-navigation", navigation_handle);
}

void WebContents::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  EmitNavigationEvent("did-redirect-navigation", navigation_handle);
}

void WebContents::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  // 不要将内容聚焦在非活动窗口中。
  if (!owner_window())
    return;
#if defined(OS_MAC)
  if (!owner_window()->IsActive())
    return;
#else
  if (!owner_window()->widget()->IsActive())
    return;
#endif
  // 子框架导航后不要聚焦内容。
  if (!navigation_handle->IsInMainFrame())
    return;
  // 只关注顶级内容。
  if (type_ != Type::kBrowserWindow)
    return;
  web_contents()->SetInitialFocus();
}

void WebContents::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (owner_window_) {
    owner_window_->NotifyLayoutWindowControlsOverlay();
  }

  if (!navigation_handle->HasCommitted())
    return;
  bool is_main_frame = navigation_handle->IsInMainFrame();
  content::RenderFrameHost* frame_host =
      navigation_handle->GetRenderFrameHost();
  int frame_process_id = -1, frame_routing_id = -1;
  if (frame_host) {
    frame_process_id = frame_host->GetProcess()->GetID();
    frame_routing_id = frame_host->GetRoutingID();
  }
  if (!navigation_handle->IsErrorPage()) {
    // FIX：下面的所有emit()调用都可能导致|this|。
    // 被销毁(由JS监听事件并调用。
    // WebContents.Destroy())。
    auto url = navigation_handle->GetURL();
    bool is_same_document = navigation_handle->IsSameDocument();
    if (is_same_document) {
      Emit("did-navigate-in-page", url, is_main_frame, frame_process_id,
           frame_routing_id);
    } else {
      const net::HttpResponseHeaders* http_response =
          navigation_handle->GetResponseHeaders();
      std::string http_status_text;
      int http_response_code = -1;
      if (http_response) {
        http_status_text = http_response->GetStatusText();
        http_response_code = http_response->response_code();
      }
      Emit("did-frame-navigate", url, http_response_code, http_status_text,
           is_main_frame, frame_process_id, frame_routing_id);
      if (is_main_frame) {
        Emit("did-navigate", url, http_response_code, http_status_text);
      }
    }
    if (IsGuest())
      Emit("load-commit", url, is_main_frame);
  } else {
    auto url = navigation_handle->GetURL();
    int code = navigation_handle->GetNetErrorCode();
    auto description = net::ErrorToShortString(code);
    Emit("did-fail-provisional-load", code, description, url, is_main_frame,
         frame_process_id, frame_routing_id);

    // 对于取消的请求，不要发出“DID-FAIL-LOAD”。
    if (code != net::ERR_ABORTED) {
      EmitWarning(
          node::Environment::GetCurrent(JavascriptEnvironment::GetIsolate()),
          "Failed to load URL: " + url.possibly_invalid_spec() +
              " with error: " + description,
          "electron");
      Emit("did-fail-load", code, description, url, is_main_frame,
           frame_process_id, frame_routing_id);
    }
  }
  content::NavigationEntry* entry = navigation_handle->GetNavigationEntry();

  // 由于Chromium中的一个问题，需要进行此检查。
  // 检查Chromium问题以保持更新：
  // Https://bugs.chromium.org/p/chromium/issues/detail?id=1178663
  // 如果已经进行了历史条目并且已经进行了前向/回拨呼叫，
  // 继续设置新标题。
  if (entry && (entry->GetTransitionType() & ui::PAGE_TRANSITION_FORWARD_BACK))
    WebContents::TitleWasSet(entry);
}

void WebContents::TitleWasSet(content::NavigationEntry* entry) {
  std::u16string final_title;
  bool explicit_set = true;
  if (entry) {
    auto title = entry->GetTitle();
    auto url = entry->GetURL();
    if (url.SchemeIsFile() && title.empty()) {
      final_title = base::UTF8ToUTF16(url.ExtractFileName());
      explicit_set = false;
    } else {
      final_title = title;
    }
  } else {
    final_title = web_contents()->GetTitle();
  }
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnPageTitleUpdated(final_title, explicit_set);
  Emit("page-title-updated", final_title, explicit_set);
}

void WebContents::DidUpdateFaviconURL(
    content::RenderFrameHost* render_frame_host,
    const std::vector<blink::mojom::FaviconURLPtr>& urls) {
  std::set<GURL> unique_urls;
  for (const auto& iter : urls) {
    if (iter->icon_type != blink::mojom::FaviconIconType::kFavicon)
      continue;
    const GURL& url = iter->icon_url;
    if (url.is_valid())
      unique_urls.insert(url);
  }
  Emit("page-favicon-updated", unique_urls);
}

void WebContents::DevToolsReloadPage() {
  Emit("devtools-reload-page");
}

void WebContents::DevToolsFocused() {
  Emit("devtools-focused");
}

void WebContents::DevToolsOpened() {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  DCHECK(inspectable_web_contents_);
  DCHECK(inspectable_web_contents_->GetDevToolsWebContents());
  auto handle = FromOrCreate(
      isolate, inspectable_web_contents_->GetDevToolsWebContents());
  devtools_web_contents_.Reset(isolate, handle.ToV8());

  // 设置已检查的TABID。
  base::Value tab_id(ID());
  inspectable_web_contents_->CallClientFunction("DevToolsAPI.setInspectedTabId",
                                                &tab_id, nullptr, nullptr);

  // 如果DevTools中没有所有者窗口，则继承所有者窗口。
  auto* devtools = inspectable_web_contents_->GetDevToolsWebContents();
  bool has_window = devtools->GetUserData(NativeWindowRelay::UserDataKey());
  if (owner_window() && !has_window)
    handle->SetOwnerWindow(devtools, owner_window());

  Emit("devtools-opened");
}

void WebContents::DevToolsClosed() {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  devtools_web_contents_.Reset();

  Emit("devtools-closed");
}

void WebContents::DevToolsResized() {
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnDevToolsResized();
}

void WebContents::SetOwnerWindow(NativeWindow* owner_window) {
  SetOwnerWindow(GetWebContents(), owner_window);
}

void WebContents::SetOwnerWindow(content::WebContents* web_contents,
                                 NativeWindow* owner_window) {
  if (owner_window) {
    owner_window_ = owner_window->GetWeakPtr();
    NativeWindowRelay::CreateForWebContents(web_contents,
                                            owner_window->GetWeakPtr());
  } else {
    owner_window_ = nullptr;
    web_contents->RemoveUserData(NativeWindowRelay::UserDataKey());
  }
#if BUILDFLAG(ENABLE_OSR)
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetNativeWindow(owner_window);
#endif
}

content::WebContents* WebContents::GetWebContents() const {
  if (!inspectable_web_contents_)
    return nullptr;
  return inspectable_web_contents_->GetWebContents();
}

content::WebContents* WebContents::GetDevToolsWebContents() const {
  if (!inspectable_web_contents_)
    return nullptr;
  return inspectable_web_contents_->GetDevToolsWebContents();
}

void WebContents::WebContentsDestroyed() {
  // 清除包装器中存储的指针。
  if (GetAllWebContents().Lookup(id_))
    GetAllWebContents().Remove(id_);
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper))
    return;
  wrapper->SetAlignedPointerInInternalField(0, nullptr);

  // 告诉WebViewGuestDelegate WebContents已销毁。
  if (guest_delegate_)
    guest_delegate_->WillDestroy();

  Observe(nullptr);
  Emit("destroyed");
}

void WebContents::NavigationEntryCommitted(
    const content::LoadCommittedDetails& details) {
  Emit("navigation-entry-committed", details.entry->GetURL(),
       details.is_same_document, details.did_replace_entry);
}

bool WebContents::GetBackgroundThrottling() const {
  return background_throttling_;
}

void WebContents::SetBackgroundThrottling(bool allowed) {
  background_throttling_ = allowed;

  auto* rfh = web_contents()->GetMainFrame();
  if (!rfh)
    return;

  auto* rwhv = rfh->GetView();
  if (!rwhv)
    return;

  auto* rwh_impl =
      static_cast<content::RenderWidgetHostImpl*>(rwhv->GetRenderWidgetHost());
  if (!rwh_impl)
    return;

  rwh_impl->disable_hidden_ = !background_throttling_;
  web_contents()->GetRenderViewHost()->SetSchedulerThrottling(allowed);

  if (rwh_impl->is_hidden()) {
    rwh_impl->WasShown({});
  }
}

int WebContents::GetProcessID() const {
  return web_contents()->GetMainFrame()->GetProcess()->GetID();
}

base::ProcessId WebContents::GetOSProcessID() const {
  base::ProcessHandle process_handle =
      web_contents()->GetMainFrame()->GetProcess()->GetProcess().Handle();
  return base::GetProcId(process_handle);
}

WebContents::Type WebContents::GetType() const {
  return type_;
}

bool WebContents::Equal(const WebContents* web_contents) const {
  return ID() == web_contents->ID();
}

GURL WebContents::GetURL() const {
  return web_contents()->GetLastCommittedURL();
}

void WebContents::LoadURL(const GURL& url,
                          const gin_helper::Dictionary& options) {
  if (!url.is_valid() || url.spec().size() > url::kMaxURLChars) {
    Emit("did-fail-load", static_cast<int>(net::ERR_INVALID_URL),
         net::ErrorToShortString(net::ERR_INVALID_URL),
         url.possibly_invalid_spec(), true);
    return;
  }

  content::NavigationController::LoadURLParams params(url);

  if (!options.Get("httpReferrer", &params.referrer)) {
    GURL http_referrer;
    if (options.Get("httpReferrer", &http_referrer))
      params.referrer =
          content::Referrer(http_referrer.GetAsReferrer(),
                            network::mojom::ReferrerPolicy::kDefault);
  }

  std::string user_agent;
  if (options.Get("userAgent", &user_agent))
    web_contents()->SetUserAgentOverride(
        blink::UserAgentOverride::UserAgentOnly(user_agent), false);

  std::string extra_headers;
  if (options.Get("extraHeaders", &extra_headers))
    params.extra_headers = extra_headers;

  scoped_refptr<network::ResourceRequestBody> body;
  if (options.Get("postData", &body)) {
    params.post_data = body;
    params.load_type = content::NavigationController::LOAD_TYPE_HTTP_POST;
  }

  GURL base_url_for_data_url;
  if (options.Get("baseURLForDataURL", &base_url_for_data_url)) {
    params.base_url_for_data_url = base_url_for_data_url;
    params.load_type = content::NavigationController::LOAD_TYPE_DATA;
  }

  bool reload_ignoring_cache = false;
  if (options.Get("reloadIgnoringCache", &reload_ignoring_cache) &&
      reload_ignoring_cache) {
    params.reload_type = content::ReloadType::BYPASSING_CACHE;
  }

  // 调用LoadURLWithParams()可以触发销毁|this|的JS。
  auto weak_this = GetWeakPtr();

  params.transition_type = ui::PageTransitionFromInt(
      ui::PAGE_TRANSITION_TYPED | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR);
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  // 不协调未提交的条目，以确保我们不会重用挂起的。
  // 条目。
  web_contents()->GetController().DiscardNonCommittedEntries();
  web_contents()->GetController().LoadURLWithParams(params);

  // ⚠️警告！⚠️。
  // LoadURLWithParams()触发JS事件，JS事件可以在|this|上调用Destroy()。
  // 在这一点上，假设|this|指向有效内存是不安全的。
  if (!weak_this || !web_contents())
    return;

  // 需要使Bere Unload处理程序工作。
  NotifyUserActivation();
}

// TODO(MarshallOfSound)：弄清楚我们需要如何处理这里的POST数据，我。
// 我相信当我们传递“true”时的默认行为是向外拨电话到。
// 委托，然后控制器期望使用以下命令再次调用此方法。
// 如果用户批准重新加载，则为“false”。就目前而言，这将导致。
// POST Data Domain上的“.reload()”调用以静默方式失败。如果传递假的话。
// 结果是他们成功了，但转发了尽管更正确的。
// 考虑一个突破性的变化。
void WebContents::Reload() {
  web_contents()->GetController().Reload(content::ReloadType::NORMAL,
                                         /* 检查是否转发。*/ true);
}

void WebContents::ReloadIgnoringCache() {
  web_contents()->GetController().Reload(content::ReloadType::BYPASSING_CACHE,
                                         /* 检查是否转发。*/ true);
}

void WebContents::DownloadURL(const GURL& url) {
  auto* browser_context = web_contents()->GetBrowserContext();
  auto* download_manager = browser_context->GetDownloadManager();
  std::unique_ptr<download::DownloadUrlParameters> download_params(
      content::DownloadRequestUtils::CreateDownloadForWebContentsMainFrame(
          web_contents(), url, MISSING_TRAFFIC_ANNOTATION));
  download_manager->DownloadUrl(std::move(download_params));
}

std::u16string WebContents::GetTitle() const {
  return web_contents()->GetTitle();
}

bool WebContents::IsLoading() const {
  return web_contents()->IsLoading();
}

bool WebContents::IsLoadingMainFrame() const {
  return web_contents()->IsLoadingToDifferentDocument();
}

bool WebContents::IsWaitingForResponse() const {
  return web_contents()->IsWaitingForResponse();
}

void WebContents::Stop() {
  web_contents()->Stop();
}

bool WebContents::CanGoBack() const {
  return web_contents()->GetController().CanGoBack();
}

void WebContents::GoBack() {
  if (CanGoBack())
    web_contents()->GetController().GoBack();
}

bool WebContents::CanGoForward() const {
  return web_contents()->GetController().CanGoForward();
}

void WebContents::GoForward() {
  if (CanGoForward())
    web_contents()->GetController().GoForward();
}

bool WebContents::CanGoToOffset(int offset) const {
  return web_contents()->GetController().CanGoToOffset(offset);
}

void WebContents::GoToOffset(int offset) {
  if (CanGoToOffset(offset))
    web_contents()->GetController().GoToOffset(offset);
}

bool WebContents::CanGoToIndex(int index) const {
  return index >= 0 && index < GetHistoryLength();
}

void WebContents::GoToIndex(int index) {
  if (CanGoToIndex(index))
    web_contents()->GetController().GoToIndex(index);
}

int WebContents::GetActiveIndex() const {
  return web_contents()->GetController().GetCurrentEntryIndex();
}

void WebContents::ClearHistory() {
  // 在一些罕见的情况下(通常没有真实的历史)，我们处于。
  // 我们不能删除导航条目的状态。
  if (web_contents()->GetController().CanPruneAllButLastCommitted()) {
    web_contents()->GetController().PruneAllButLastCommitted();
  }
}

int WebContents::GetHistoryLength() const {
  return web_contents()->GetController().GetEntryCount();
}

const std::string WebContents::GetWebRTCIPHandlingPolicy() const {
  return web_contents()->GetMutableRendererPrefs()->webrtc_ip_handling_policy;
}

void WebContents::SetWebRTCIPHandlingPolicy(
    const std::string& webrtc_ip_handling_policy) {
  if (GetWebRTCIPHandlingPolicy() == webrtc_ip_handling_policy)
    return;
  web_contents()->GetMutableRendererPrefs()->webrtc_ip_handling_policy =
      webrtc_ip_handling_policy;

  web_contents()->SyncRendererPrefs();
}

bool WebContents::IsCrashed() const {
  return web_contents()->IsCrashed();
}

void WebContents::ForcefullyCrashRenderer() {
  content::RenderWidgetHostView* view =
      web_contents()->GetRenderWidgetHostView();
  if (!view)
    return;

  content::RenderWidgetHost* rwh = view->GetRenderWidgetHost();
  if (!rwh)
    return;

  content::RenderProcessHost* rph = rwh->GetProcess();
  if (rph) {
#if defined(OS_LINUX) || defined(OS_CHROMEOS)
    // Linux没有实现泛型|CrashDump匈牙利ChildProcess()|。
    // 相反，我们发送一个显式IPC，使其在呈现器的IO线程上崩溃。
    rph->ForceCrash();
#else
    // 尝试为挂起的进程生成崩溃报告。
#ifndef MAS_BUILD
    CrashDumpHungChildProcess(rph->GetProcess().Handle());
#endif
    rph->Shutdown(content::RESULT_CODE_HUNG);
#endif
  }
}

void WebContents::SetUserAgent(const std::string& user_agent) {
  web_contents()->SetUserAgentOverride(
      blink::UserAgentOverride::UserAgentOnly(user_agent), false);
}

std::string WebContents::GetUserAgent() {
  return web_contents()->GetUserAgentOverride().ua_string_override;
}

v8::Local<v8::Promise> WebContents::SavePage(
    const base::FilePath& full_file_path,
    const content::SavePageType& save_type) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* handler = new SavePageHandler(web_contents(), std::move(promise));
  handler->Handle(full_file_path, save_type);

  return handle;
}

void WebContents::OpenDevTools(gin::Arguments* args) {
  if (type_ == Type::kRemote)
    return;

  if (!enable_devtools_)
    return;

  std::string state;
  if (type_ == Type::kWebView || type_ == Type::kBackgroundPage ||
      !owner_window()) {
    state = "detach";
  }
  bool activate = true;
  if (args && args->Length() == 1) {
    gin_helper::Dictionary options;
    if (args->GetNext(&options)) {
      options.Get("mode", &state);
      options.Get("activate", &activate);
    }
  }

  DCHECK(inspectable_web_contents_);
  inspectable_web_contents_->SetDockState(state);
  inspectable_web_contents_->ShowDevTools(activate);
}

void WebContents::CloseDevTools() {
  if (type_ == Type::kRemote)
    return;

  DCHECK(inspectable_web_contents_);
  inspectable_web_contents_->CloseDevTools();
}

bool WebContents::IsDevToolsOpened() {
  if (type_ == Type::kRemote)
    return false;

  DCHECK(inspectable_web_contents_);
  return inspectable_web_contents_->IsDevToolsViewShowing();
}

bool WebContents::IsDevToolsFocused() {
  if (type_ == Type::kRemote)
    return false;

  DCHECK(inspectable_web_contents_);
  return inspectable_web_contents_->GetView()->IsDevToolsViewFocused();
}

void WebContents::EnableDeviceEmulation(
    const blink::DeviceEmulationParams& params) {
  if (type_ == Type::kRemote)
    return;

  DCHECK(web_contents());
  auto* frame_host = web_contents()->GetMainFrame();
  if (frame_host) {
    auto* widget_host_impl = static_cast<content::RenderWidgetHostImpl*>(
        frame_host->GetView()->GetRenderWidgetHost());
    if (widget_host_impl) {
      auto& frame_widget = widget_host_impl->GetAssociatedFrameWidget();
      frame_widget->EnableDeviceEmulation(params);
    }
  }
}

void WebContents::DisableDeviceEmulation() {
  if (type_ == Type::kRemote)
    return;

  DCHECK(web_contents());
  auto* frame_host = web_contents()->GetMainFrame();
  if (frame_host) {
    auto* widget_host_impl = static_cast<content::RenderWidgetHostImpl*>(
        frame_host->GetView()->GetRenderWidgetHost());
    if (widget_host_impl) {
      auto& frame_widget = widget_host_impl->GetAssociatedFrameWidget();
      frame_widget->DisableDeviceEmulation();
    }
  }
}

void WebContents::ToggleDevTools() {
  if (IsDevToolsOpened())
    CloseDevTools();
  else
    OpenDevTools(nullptr);
}

void WebContents::InspectElement(int x, int y) {
  if (type_ == Type::kRemote)
    return;

  if (!enable_devtools_)
    return;

  DCHECK(inspectable_web_contents_);
  if (!inspectable_web_contents_->GetDevToolsWebContents())
    OpenDevTools(nullptr);
  inspectable_web_contents_->InspectElement(x, y);
}

void WebContents::InspectSharedWorkerById(const std::string& workerId) {
  if (type_ == Type::kRemote)
    return;

  if (!enable_devtools_)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::kTypeSharedWorker) {
      if (agent_host->GetId() == workerId) {
        OpenDevTools(nullptr);
        inspectable_web_contents_->AttachTo(agent_host);
        break;
      }
    }
  }
}

std::vector<scoped_refptr<content::DevToolsAgentHost>>
WebContents::GetAllSharedWorkers() {
  std::vector<scoped_refptr<content::DevToolsAgentHost>> shared_workers;

  if (type_ == Type::kRemote)
    return shared_workers;

  if (!enable_devtools_)
    return shared_workers;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::kTypeSharedWorker) {
      shared_workers.push_back(agent_host);
    }
  }
  return shared_workers;
}

void WebContents::InspectSharedWorker() {
  if (type_ == Type::kRemote)
    return;

  if (!enable_devtools_)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::kTypeSharedWorker) {
      OpenDevTools(nullptr);
      inspectable_web_contents_->AttachTo(agent_host);
      break;
    }
  }
}

void WebContents::InspectServiceWorker() {
  if (type_ == Type::kRemote)
    return;

  if (!enable_devtools_)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::kTypeServiceWorker) {
      OpenDevTools(nullptr);
      inspectable_web_contents_->AttachTo(agent_host);
      break;
    }
  }
}

void WebContents::SetIgnoreMenuShortcuts(bool ignore) {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  DCHECK(web_preferences);
  web_preferences->SetIgnoreMenuShortcuts(ignore);
}

void WebContents::SetAudioMuted(bool muted) {
  web_contents()->SetAudioMuted(muted);
}

bool WebContents::IsAudioMuted() {
  return web_contents()->IsAudioMuted();
}

bool WebContents::IsCurrentlyAudible() {
  return web_contents()->IsCurrentlyAudible();
}

#if BUILDFLAG(ENABLE_PRINTING)
void WebContents::OnGetDefaultPrinter(
    base::Value print_settings,
    printing::CompletionCallback print_callback,
    std::u16string device_name,
    bool silent,
    // &lt;错误，DEFAULT_PRINTER&gt;。
    std::pair<std::string, std::u16string> info) {
  // 此时内容：：WebContents可能已被删除，并且。
  // PrintViewManagerElectron类不执行NULL检查。
  if (!web_contents()) {
    if (print_callback)
      std::move(print_callback).Run(false, "failed");
    return;
  }

  if (!info.first.empty()) {
    if (print_callback)
      std::move(print_callback).Run(false, info.first);
    return;
  }

  // 如果用户传递了deviceName，则使用它，否则使用默认打印机。
  std::u16string printer_name = device_name.empty() ? info.second : device_name;

  // 如果网络上没有可用的有效打印机，我们就退出。
  if (printer_name.empty() || !IsDeviceNameValid(printer_name)) {
    if (print_callback)
      std::move(print_callback).Run(false, "no valid printers available");
    return;
  }

  print_settings.SetStringKey(printing::kSettingDeviceName, printer_name);

  auto* print_view_manager =
      PrintViewManagerElectron::FromWebContents(web_contents());
  if (!print_view_manager)
    return;

  auto* focused_frame = web_contents()->GetFocusedFrame();
  auto* rfh = focused_frame && focused_frame->HasSelection()
                  ? focused_frame
                  : web_contents()->GetMainFrame();

  print_view_manager->PrintNow(rfh, silent, std::move(print_settings),
                               std::move(print_callback));
}

void WebContents::Print(gin::Arguments* args) {
  gin_helper::Dictionary options =
      gin::Dictionary::CreateEmpty(args->isolate());
  base::Value settings(base::Value::Type::DICTIONARY);

  if (args->Length() >= 1 && !args->GetNext(&options)) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("webContents.print(): Invalid print settings specified.");
    return;
  }

  printing::CompletionCallback callback;
  if (args->Length() == 2 && !args->GetNext(&callback)) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("webContents.print(): Invalid optional callback provided.");
    return;
  }

  // 设置可选的静默打印。
  bool silent = false;
  options.Get("silent", &silent);

  bool print_background = false;
  options.Get("printBackground", &print_background);
  settings.SetBoolKey(printing::kSettingShouldPrintBackgrounds,
                      print_background);

  // 设置自定义边距设置。
  gin_helper::Dictionary margins =
      gin::Dictionary::CreateEmpty(args->isolate());
  if (options.Get("margins", &margins)) {
    printing::mojom::MarginType margin_type =
        printing::mojom::MarginType::kDefaultMargins;
    margins.Get("marginType", &margin_type);
    settings.SetIntKey(printing::kSettingMarginsType,
                       static_cast<int>(margin_type));

    if (margin_type == printing::mojom::MarginType::kCustomMargins) {
      base::Value custom_margins(base::Value::Type::DICTIONARY);
      int top = 0;
      margins.Get("top", &top);
      custom_margins.SetIntKey(printing::kSettingMarginTop, top);
      int bottom = 0;
      margins.Get("bottom", &bottom);
      custom_margins.SetIntKey(printing::kSettingMarginBottom, bottom);
      int left = 0;
      margins.Get("left", &left);
      custom_margins.SetIntKey(printing::kSettingMarginLeft, left);
      int right = 0;
      margins.Get("right", &right);
      custom_margins.SetIntKey(printing::kSettingMarginRight, right);
      settings.SetPath(printing::kSettingMarginsCustom,
                       std::move(custom_margins));
    }
  } else {
    settings.SetIntKey(
        printing::kSettingMarginsType,
        static_cast<int>(printing::mojom::MarginType::kDefaultMargins));
  }

  // 设置是打印彩色还是灰度。
  bool print_color = true;
  options.Get("color", &print_color);
  auto const color_model = print_color ? printing::mojom::ColorModel::kColor
                                       : printing::mojom::ColorModel::kGray;
  settings.SetIntKey(printing::kSettingColor, static_cast<int>(color_model));

  // 是朝向、横向还是纵向。
  bool landscape = false;
  options.Get("landscape", &landscape);
  settings.SetBoolKey(printing::kSettingLandscape, landscape);

  // 我们将默认打印机设置为系统的默认打印机，并且仅更新。
  // 如果用户覆盖，则为Chromium级别。
  // 操作系统打开的打印机设备名称。
  std::u16string device_name;
  options.Get("deviceName", &device_name);
  if (!device_name.empty() && !IsDeviceNameValid(device_name)) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("webContents.print(): Invalid deviceName provided.");
    return;
  }

  int scale_factor = 100;
  options.Get("scaleFactor", &scale_factor);
  settings.SetIntKey(printing::kSettingScaleFactor, scale_factor);

  int pages_per_sheet = 1;
  options.Get("pagesPerSheet", &pages_per_sheet);
  settings.SetIntKey(printing::kSettingPagesPerSheet, pages_per_sheet);

  // 如果用户要使用Colate打印，则为True。
  bool collate = true;
  options.Get("collate", &collate);
  settings.SetBoolKey(printing::kSettingCollate, collate);

  // 要打印的单个份数。
  int copies = 1;
  options.Get("copies", &copies);
  settings.SetIntKey(printing::kSettingCopies, copies);

  // 如果用户请求，要打印为页眉和页脚的字符串。
  std::string header;
  options.Get("header", &header);
  std::string footer;
  options.Get("footer", &footer);

  if (!(header.empty() && footer.empty())) {
    settings.SetBoolKey(printing::kSettingHeaderFooterEnabled, true);

    settings.SetStringKey(printing::kSettingHeaderFooterTitle, header);
    settings.SetStringKey(printing::kSettingHeaderFooterURL, footer);
  } else {
    settings.SetBoolKey(printing::kSettingHeaderFooterEnabled, false);
  }

  // 我们不希望允许用户启用这些设置。
  // 但是我们需要设置它们，否则支票就会被击中。
  settings.SetIntKey(printing::kSettingPrinterType,
                     static_cast<int>(printing::mojom::PrinterType::kLocal));
  settings.SetBoolKey(printing::kSettingShouldPrintSelectionOnly, false);
  settings.SetBoolKey(printing::kSettingRasterizePdf, false);

  // 设置要打印的自定义页面范围。
  std::vector<gin_helper::Dictionary> page_ranges;
  if (options.Get("pageRanges", &page_ranges)) {
    base::Value page_range_list(base::Value::Type::LIST);
    for (auto& range : page_ranges) {
      int from, to;
      if (range.Get("from", &from) && range.Get("to", &to)) {
        base::Value range(base::Value::Type::DICTIONARY);
        // Chrome使用以1为基数的页面范围，因此每个页面范围递增1。
        range.SetIntKey(printing::kSettingPageRangeFrom, from + 1);
        range.SetIntKey(printing::kSettingPageRangeTo, to + 1);
        page_range_list.Append(std::move(range));
      } else {
        continue;
      }
    }
    if (!page_range_list.GetList().empty())
      settings.SetPath(printing::kSettingPageRange, std::move(page_range_list));
  }

  // 用户要使用的双工类型。
  printing::mojom::DuplexMode duplex_mode =
      printing::mojom::DuplexMode::kSimplex;
  options.Get("duplexMode", &duplex_mode);
  settings.SetIntKey(printing::kSettingDuplexMode,
                     static_cast<int>(duplex_mode));

  // 我们已经在。
  // JS级别，所以我们可以简单地通过它。
  base::Value media_size(base::Value::Type::DICTIONARY);
  if (options.Get("mediaSize", &media_size))
    settings.SetKey(printing::kSettingMediaSize, std::move(media_size));

  // 设置自定义每英寸点数(Dpi)。
  gin_helper::Dictionary dpi_settings;
  int dpi = 72;
  if (options.Get("dpi", &dpi_settings)) {
    int horizontal = 72;
    dpi_settings.Get("horizontal", &horizontal);
    settings.SetIntKey(printing::kSettingDpiHorizontal, horizontal);
    int vertical = 72;
    dpi_settings.Get("vertical", &vertical);
    settings.SetIntKey(printing::kSettingDpiVertical, vertical);
  } else {
    settings.SetIntKey(printing::kSettingDpiHorizontal, dpi);
    settings.SetIntKey(printing::kSettingDpiVertical, dpi);
  }

  print_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&GetDefaultPrinterAsync),
      base::BindOnce(&WebContents::OnGetDefaultPrinter,
                     weak_factory_.GetWeakPtr(), std::move(settings),
                     std::move(callback), device_name, silent));
}

v8::Local<v8::Promise> WebContents::PrintToPDF(base::DictionaryValue settings) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  PrintPreviewMessageHandler::FromWebContents(web_contents())
      ->PrintToPDF(std::move(settings), std::move(promise));
  return handle;
}
#endif

void WebContents::AddWorkSpace(gin::Arguments* args,
                               const base::FilePath& path) {
  if (path.empty()) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("path cannot be empty");
    return;
  }
  DevToolsAddFileSystem(std::string(), path);
}

void WebContents::RemoveWorkSpace(gin::Arguments* args,
                                  const base::FilePath& path) {
  if (path.empty()) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("path cannot be empty");
    return;
  }
  DevToolsRemoveFileSystem(path);
}

void WebContents::Undo() {
  web_contents()->Undo();
}

void WebContents::Redo() {
  web_contents()->Redo();
}

void WebContents::Cut() {
  web_contents()->Cut();
}

void WebContents::Copy() {
  web_contents()->Copy();
}

void WebContents::Paste() {
  web_contents()->Paste();
}

void WebContents::PasteAndMatchStyle() {
  web_contents()->PasteAndMatchStyle();
}

void WebContents::Delete() {
  web_contents()->Delete();
}

void WebContents::SelectAll() {
  web_contents()->SelectAll();
}

void WebContents::Unselect() {
  web_contents()->CollapseSelection();
}

void WebContents::Replace(const std::u16string& word) {
  web_contents()->Replace(word);
}

void WebContents::ReplaceMisspelling(const std::u16string& word) {
  web_contents()->ReplaceMisspelling(word);
}

uint32_t WebContents::FindInPage(gin::Arguments* args) {
  std::u16string search_text;
  if (!args->GetNext(&search_text) || search_text.empty()) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("Must provide a non-empty search content");
    return 0;
  }

  uint32_t request_id = ++find_in_page_request_id_;
  gin_helper::Dictionary dict;
  auto options = blink::mojom::FindOptions::New();
  if (args->GetNext(&dict)) {
    dict.Get("forward", &options->forward);
    dict.Get("matchCase", &options->match_case);
    dict.Get("findNext", &options->new_session);
  }

  web_contents()->Find(request_id, search_text, std::move(options));
  return request_id;
}

void WebContents::StopFindInPage(content::StopFindAction action) {
  web_contents()->StopFinding(action);
}

void WebContents::ShowDefinitionForSelection() {
#if defined(OS_MAC)
  auto* const view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->ShowDefinitionForSelection();
#endif
}

void WebContents::CopyImageAt(int x, int y) {
  auto* const host = web_contents()->GetMainFrame();
  if (host)
    host->CopyImageAt(x, y);
}

void WebContents::Focus() {
  // 聚焦于WebContents不会自动将窗口聚焦在MacOS上。
  // 和Linux，请手动执行此操作，以与Windows上的行为相匹配。
#if defined(OS_MAC) || defined(OS_LINUX)
  if (owner_window())
    owner_window()->Focus(true);
#endif
  web_contents()->Focus();
}

#if !defined(OS_MAC)
bool WebContents::IsFocused() const {
  auto* view = web_contents()->GetRenderWidgetHostView();
  if (!view)
    return false;

  if (GetType() != Type::kBackgroundPage) {
    auto* window = web_contents()->GetNativeView()->GetToplevelWindow();
    if (window && !window->IsVisible())
      return false;
  }

  return view->HasFocus();
}
#endif

void WebContents::SendInputEvent(v8::Isolate* isolate,
                                 v8::Local<v8::Value> input_event) {
  content::RenderWidgetHostView* view =
      web_contents()->GetRenderWidgetHostView();
  if (!view)
    return;

  content::RenderWidgetHost* rwh = view->GetRenderWidgetHost();
  blink::WebInputEvent::Type type =
      gin::GetWebInputEventType(isolate, input_event);
  if (blink::WebInputEvent::IsMouseEventType(type)) {
    blink::WebMouseEvent mouse_event;
    if (gin::ConvertFromV8(isolate, input_event, &mouse_event)) {
      if (IsOffScreen()) {
#if BUILDFLAG(ENABLE_OSR)
        GetOffScreenRenderWidgetHostView()->SendMouseEvent(mouse_event);
#endif
      } else {
        rwh->ForwardMouseEvent(mouse_event);
      }
      return;
    }
  } else if (blink::WebInputEvent::IsKeyboardEventType(type)) {
    content::NativeWebKeyboardEvent keyboard_event(
        blink::WebKeyboardEvent::Type::kRawKeyDown,
        blink::WebInputEvent::Modifiers::kNoModifiers, ui::EventTimeForNow());
    if (gin::ConvertFromV8(isolate, input_event, &keyboard_event)) {
      rwh->ForwardKeyboardEvent(keyboard_event);
      return;
    }
  } else if (type == blink::WebInputEvent::Type::kMouseWheel) {
    blink::WebMouseWheelEvent mouse_wheel_event;
    if (gin::ConvertFromV8(isolate, input_event, &mouse_wheel_event)) {
      if (IsOffScreen()) {
#if BUILDFLAG(ENABLE_OSR)
        GetOffScreenRenderWidgetHostView()->SendMouseWheelEvent(
            mouse_wheel_event);
#endif
      } else {
        // 铬需要轮子事件中的阶段信息(并应用。
        // DCHECK来验证它)。请参阅：https://crbug.com/756524.。
        mouse_wheel_event.phase = blink::WebMouseWheelEvent::kPhaseBegan;
        mouse_wheel_event.dispatch_type =
            blink::WebInputEvent::DispatchType::kBlocking;
        rwh->ForwardWheelEvent(mouse_wheel_event);

        // 发送带有phaseEnded的合成滚轮事件以完成滚动。
        mouse_wheel_event.has_synthetic_phase = true;
        mouse_wheel_event.delta_x = 0;
        mouse_wheel_event.delta_y = 0;
        mouse_wheel_event.phase = blink::WebMouseWheelEvent::kPhaseEnded;
        mouse_wheel_event.dispatch_type =
            blink::WebInputEvent::DispatchType::kEventNonBlocking;
        rwh->ForwardWheelEvent(mouse_wheel_event);
      }
      return;
    }
  }

  isolate->ThrowException(
      v8::Exception::Error(gin::StringToV8(isolate, "Invalid event object")));
}

void WebContents::BeginFrameSubscription(gin::Arguments* args) {
  bool only_dirty = false;
  FrameSubscriber::FrameCaptureCallback callback;

  if (args->Length() > 1) {
    if (!args->GetNext(&only_dirty)) {
      args->ThrowError();
      return;
    }
  }
  if (!args->GetNext(&callback)) {
    args->ThrowError();
    return;
  }

  frame_subscriber_ =
      std::make_unique<FrameSubscriber>(web_contents(), callback, only_dirty);
}

void WebContents::EndFrameSubscription() {
  frame_subscriber_.reset();
}

void WebContents::StartDrag(const gin_helper::Dictionary& item,
                            gin::Arguments* args) {
  base::FilePath file;
  std::vector<base::FilePath> files;
  if (!item.Get("files", &files) && item.Get("file", &file)) {
    files.push_back(file);
  }

  v8::Local<v8::Value> icon_value;
  if (!item.Get("icon", &icon_value)) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("'icon' parameter is required");
    return;
  }

  NativeImage* icon = nullptr;
  if (!NativeImage::TryConvertNativeImage(args->isolate(), icon_value, &icon) ||
      icon->image().IsEmpty()) {
    return;
  }

  // 开始拖拽。
  if (!files.empty()) {
    base::CurrentThread::ScopedNestableTaskAllower allow;
    DragFileItems(files, icon->image(), web_contents()->GetNativeView());
  } else {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("Must specify either 'file' or 'files' option");
  }
}

v8::Local<v8::Promise> WebContents::CapturePage(gin::Arguments* args) {
  gfx::Rect rect;
  gin_helper::Promise<gfx::Image> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // 获取RECT参数(如果存在)。
  args->GetNext(&rect);

  auto* const view = web_contents()->GetRenderWidgetHostView();
  if (!view) {
    promise.Resolve(gfx::Image());
    return handle;
  }

#if !defined(OS_MAC)
  // 如果视图的呈现器挂起，则在Windows/Linux上可能会失败-。
  // 如果是的话，可以保释。请参见中的CopyFromSurface。
  // Content/public/browser/render_widget_host_view.h.
  auto* rfh = web_contents()->GetMainFrame();
  if (rfh &&
      rfh->GetVisibilityState() == blink::mojom::PageVisibilityState::kHidden) {
    promise.Resolve(gfx::Image());
    return handle;
  }
#endif  // 已定义(OS_MAC)。

  // 如果用户未指定|RECT|，则捕获整个页面。
  const gfx::Size view_size =
      rect.IsEmpty() ? view->GetViewBounds().size() : rect.size();

  // 默认情况下，请求的位图大小为屏幕中的视图大小。
  // 坐标。但是，如果有更多像素细节可用，
  // 当前系统，请增加请求的位图大小以捕获所有内容。
  gfx::Size bitmap_size = view_size;
  const gfx::NativeView native_view = view->GetNativeView();
  const float scale = display::Screen::GetScreen()
                          ->GetDisplayNearestView(native_view)
                          .device_scale_factor();
  if (scale > 1.0f)
    bitmap_size = gfx::ScaleToCeiledSize(view_size, scale);

  view->CopyFromSurface(gfx::Rect(rect.origin(), view_size), bitmap_size,
                        base::BindOnce(&OnCapturePageDone, std::move(promise)));
  return handle;
}

void WebContents::IncrementCapturerCount(gin::Arguments* args) {
  gfx::Size size;
  bool stay_hidden = false;
  bool stay_awake = false;

  // 获取大小参数(如果存在)。
  args->GetNext(&size);
  // 获取stayHidden参数(如果存在)。
  args->GetNext(&stay_hidden);
  // 获取stayAwake参数(如果存在)。
  args->GetNext(&stay_awake);

  ignore_result(
      web_contents()->IncrementCapturerCount(size, stay_hidden, stay_awake));
}

void WebContents::DecrementCapturerCount(gin::Arguments* args) {
  bool stay_hidden = false;
  bool stay_awake = false;

  // 获取stayHidden参数(如果存在)。
  args->GetNext(&stay_hidden);
  // 获取stayAwake参数(如果存在)。
  args->GetNext(&stay_awake);

  web_contents()->DecrementCapturerCount(stay_hidden, stay_awake);
}

bool WebContents::IsBeingCaptured() {
  return web_contents()->IsBeingCaptured();
}

void WebContents::OnCursorChanged(const content::WebCursor& webcursor) {
  const ui::Cursor& cursor = webcursor.cursor();

  if (cursor.type() == ui::mojom::CursorType::kCustom) {
    Emit("cursor-changed", CursorTypeToString(cursor),
         gfx::Image::CreateFrom1xBitmap(cursor.custom_bitmap()),
         cursor.image_scale_factor(),
         gfx::Size(cursor.custom_bitmap().width(),
                   cursor.custom_bitmap().height()),
         cursor.custom_hotspot());
  } else {
    Emit("cursor-changed", CursorTypeToString(cursor));
  }
}

bool WebContents::IsGuest() const {
  return type_ == Type::kWebView;
}

void WebContents::AttachToIframe(content::WebContents* embedder_web_contents,
                                 int embedder_frame_id) {
  attached_ = true;
  if (guest_delegate_)
    guest_delegate_->AttachToIframe(embedder_web_contents, embedder_frame_id);
}

bool WebContents::IsOffScreen() const {
#if BUILDFLAG(ENABLE_OSR)
  return type_ == Type::kOffScreen;
#else
  return false;
#endif
}

#if BUILDFLAG(ENABLE_OSR)
void WebContents::OnPaint(const gfx::Rect& dirty_rect, const SkBitmap& bitmap) {
  Emit("paint", dirty_rect, gfx::Image::CreateFrom1xBitmap(bitmap));
}

void WebContents::StartPainting() {
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetPainting(true);
}

void WebContents::StopPainting() {
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetPainting(false);
}

bool WebContents::IsPainting() const {
  auto* osr_wcv = GetOffScreenWebContentsView();
  return osr_wcv && osr_wcv->IsPainting();
}

void WebContents::SetFrameRate(int frame_rate) {
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetFrameRate(frame_rate);
}

int WebContents::GetFrameRate() const {
  auto* osr_wcv = GetOffScreenWebContentsView();
  return osr_wcv ? osr_wcv->GetFrameRate() : 0;
}
#endif

void WebContents::Invalidate() {
  if (IsOffScreen()) {
#if BUILDFLAG(ENABLE_OSR)
    auto* osr_rwhv = GetOffScreenRenderWidgetHostView();
    if (osr_rwhv)
      osr_rwhv->Invalidate();
#endif
  } else {
    auto* const window = owner_window();
    if (window)
      window->Invalidate();
  }
}

gfx::Size WebContents::GetSizeForNewRenderView(content::WebContents* wc) {
  if (IsOffScreen() && wc == web_contents()) {
    auto* relay = NativeWindowRelay::FromWebContents(web_contents());
    if (relay) {
      auto* owner_window = relay->GetNativeWindow();
      return owner_window ? owner_window->GetSize() : gfx::Size();
    }
  }

  return gfx::Size();
}

void WebContents::SetZoomLevel(double level) {
  zoom_controller_->SetZoomLevel(level);
}

double WebContents::GetZoomLevel() const {
  return zoom_controller_->GetZoomLevel();
}

void WebContents::SetZoomFactor(gin_helper::ErrorThrower thrower,
                                double factor) {
  if (factor < std::numeric_limits<double>::epsilon()) {
    thrower.ThrowError("'zoomFactor' must be a double greater than 0.0");
    return;
  }

  auto level = blink::PageZoomFactorToZoomLevel(factor);
  SetZoomLevel(level);
}

double WebContents::GetZoomFactor() const {
  auto level = GetZoomLevel();
  return blink::PageZoomLevelToZoomFactor(level);
}

void WebContents::SetTemporaryZoomLevel(double level) {
  zoom_controller_->SetTemporaryZoomLevel(level);
}

void WebContents::DoGetZoomLevel(
    electron::mojom::ElectronBrowser::DoGetZoomLevelCallback callback) {
  std::move(callback).Run(GetZoomLevel());
}

std::vector<base::FilePath> WebContents::GetPreloadPaths() const {
  auto result = SessionPreferences::GetValidPreloads(GetBrowserContext());

  if (auto* web_preferences = WebContentsPreferences::From(web_contents())) {
    base::FilePath preload;
    if (web_preferences->GetPreloadPath(&preload)) {
      result.emplace_back(preload);
    }
  }

  return result;
}

v8::Local<v8::Value> WebContents::GetLastWebPreferences(
    v8::Isolate* isolate) const {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  if (!web_preferences)
    return v8::Null(isolate);
  return gin::ConvertToV8(isolate, *web_preferences->last_preference());
}

v8::Local<v8::Value> WebContents::GetOwnerBrowserWindow(
    v8::Isolate* isolate) const {
  if (owner_window())
    return BrowserWindow::From(isolate, owner_window());
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> WebContents::Session(v8::Isolate* isolate) {
  return v8::Local<v8::Value>::New(isolate, session_);
}

content::WebContents* WebContents::HostWebContents() const {
  if (!embedder_)
    return nullptr;
  return embedder_->web_contents();
}

void WebContents::SetEmbedder(const WebContents* embedder) {
  if (embedder) {
    NativeWindow* owner_window = nullptr;
    auto* relay = NativeWindowRelay::FromWebContents(embedder->web_contents());
    if (relay) {
      owner_window = relay->GetNativeWindow();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);

    content::RenderWidgetHostView* rwhv =
        web_contents()->GetRenderWidgetHostView();
    if (rwhv) {
      rwhv->Hide();
      rwhv->Show();
    }
  }
}

void WebContents::SetDevToolsWebContents(const WebContents* devtools) {
  if (inspectable_web_contents_)
    inspectable_web_contents_->SetDevToolsWebContents(devtools->web_contents());
}

v8::Local<v8::Value> WebContents::GetNativeView(v8::Isolate* isolate) const {
  gfx::NativeView ptr = web_contents()->GetNativeView();
  auto buffer = node::Buffer::Copy(isolate, reinterpret_cast<char*>(&ptr),
                                   sizeof(gfx::NativeView));
  if (buffer.IsEmpty())
    return v8::Null(isolate);
  else
    return buffer.ToLocalChecked();
}

v8::Local<v8::Value> WebContents::DevToolsWebContents(v8::Isolate* isolate) {
  if (devtools_web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, devtools_web_contents_);
}

v8::Local<v8::Value> WebContents::Debugger(v8::Isolate* isolate) {
  if (debugger_.IsEmpty()) {
    auto handle = electron::api::Debugger::Create(isolate, web_contents());
    debugger_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, debugger_);
}

content::RenderFrameHost* WebContents::MainFrame() {
  return web_contents()->GetMainFrame();
}

void WebContents::NotifyUserActivation() {
  content::RenderFrameHost* frame = web_contents()->GetMainFrame();
  if (frame)
    frame->NotifyUserActivation(
        blink::mojom::UserActivationNotificationType::kInteraction);
}

void WebContents::SetImageAnimationPolicy(const std::string& new_policy) {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  web_preferences->SetImageAnimationPolicy(new_policy);
  web_contents()->OnWebPreferencesChanged();
}

v8::Local<v8::Promise> WebContents::GetProcessMemoryInfo(v8::Isolate* isolate) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* frame_host = web_contents()->GetMainFrame();
  if (!frame_host) {
    promise.RejectWithErrorMessage("Failed to create memory dump");
    return handle;
  }

  auto pid = frame_host->GetProcess()->GetProcess().Pid();
  v8::Global<v8::Context> context(isolate, isolate->GetCurrentContext());
  memory_instrumentation::MemoryInstrumentation::GetInstance()
      ->RequestGlobalDumpForPid(
          pid, std::vector<std::string>(),
          base::BindOnce(&ElectronBindings::DidReceiveMemoryDump,
                         std::move(context), std::move(promise), pid));
  return handle;
}

v8::Local<v8::Promise> WebContents::TakeHeapSnapshot(
    v8::Isolate* isolate,
    const base::FilePath& file_path) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::File file(file_path,
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid()) {
    promise.RejectWithErrorMessage("takeHeapSnapshot failed");
    return handle;
  }

  auto* frame_host = web_contents()->GetMainFrame();
  if (!frame_host) {
    promise.RejectWithErrorMessage("takeHeapSnapshot failed");
    return handle;
  }

  if (!frame_host->IsRenderFrameCreated()) {
    promise.RejectWithErrorMessage("takeHeapSnapshot failed");
    return handle;
  }

  // 这个与`base：：owned`的舞蹈是为了确保接口保持活动状态。
  // 直到调用回调。否则，它将在。
  // 此函数。
  auto electron_renderer =
      std::make_unique<mojo::Remote<mojom::ElectronRenderer>>();
  frame_host->GetRemoteInterfaces()->GetInterface(
      electron_renderer->BindNewPipeAndPassReceiver());
  auto* raw_ptr = electron_renderer.get();
  (*raw_ptr)->TakeHeapSnapshot(
      mojo::WrapPlatformFile(base::ScopedPlatformFile(file.TakePlatformFile())),
      base::BindOnce(
          [](mojo::Remote<mojom::ElectronRenderer>* ep,
             gin_helper::Promise<void> promise, bool success) {
            if (success) {
              promise.Resolve();
            } else {
              promise.RejectWithErrorMessage("takeHeapSnapshot failed");
            }
          },
          base::Owned(std::move(electron_renderer)), std::move(promise)));
  return handle;
}

void WebContents::GrantDevicePermission(
    const url::Origin& origin,
    const base::Value* device,
    content::PermissionType permissionType,
    content::RenderFrameHost* render_frame_host) {
  granted_devices_[render_frame_host->GetFrameTreeNodeId()][permissionType]
                  [origin]
                      .push_back(
                          std::make_unique<base::Value>(device->Clone()));
}

std::vector<base::Value> WebContents::GetGrantedDevices(
    const url::Origin& origin,
    content::PermissionType permissionType,
    content::RenderFrameHost* render_frame_host) {
  const auto& devices_for_frame_host_it =
      granted_devices_.find(render_frame_host->GetFrameTreeNodeId());
  if (devices_for_frame_host_it == granted_devices_.end())
    return {};

  const auto& current_devices_it =
      devices_for_frame_host_it->second.find(permissionType);
  if (current_devices_it == devices_for_frame_host_it->second.end())
    return {};

  const auto& origin_devices_it = current_devices_it->second.find(origin);
  if (origin_devices_it == current_devices_it->second.end())
    return {};

  std::vector<base::Value> results;
  for (const auto& object : origin_devices_it->second)
    results.push_back(object->Clone());

  return results;
}

void WebContents::UpdatePreferredSize(content::WebContents* web_contents,
                                      const gfx::Size& pref_size) {
  Emit("preferred-size-changed", pref_size);
}

bool WebContents::CanOverscrollContent() {
  return false;
}

std::unique_ptr<content::EyeDropper> WebContents::OpenEyeDropper(
    content::RenderFrameHost* frame,
    content::EyeDropperListener* listener) {
  return ShowEyeDropper(frame, listener);
}

void WebContents::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params) {
  FileSelectHelper::RunFileChooser(render_frame_host, std::move(listener),
                                   params);
}

void WebContents::EnumerateDirectory(
    content::WebContents* web_contents,
    scoped_refptr<content::FileSelectListener> listener,
    const base::FilePath& path) {
  FileSelectHelper::EnumerateDirectory(web_contents, std::move(listener), path);
}

bool WebContents::IsFullscreenForTabOrPending(
    const content::WebContents* source) {
  return html_fullscreen_;
}

blink::SecurityStyle WebContents::GetSecurityStyle(
    content::WebContents* web_contents,
    content::SecurityStyleExplanations* security_style_explanations) {
  auto state = security_state::GetVisibleSecurityState(web_contents);
  auto security_level = security_state::GetSecurityLevel(*state, false);
  return security_state::GetSecurityStyle(security_level, *state,
                                          security_style_explanations);
}

bool WebContents::TakeFocus(content::WebContents* source, bool reverse) {
  if (source && source->GetOutermostWebContents() == source) {
    // 如果这是最外层的Web内容，并且用户已标记或。
    // 按住Shift+Tab键在所有元素之间切换，将焦点重置回。
    // 第一个或最后一个元素，这样它就不会留在身体里。
    source->FocusThroughTabTraversal(reverse);
    return true;
  }

  return false;
}

content::PictureInPictureResult WebContents::EnterPictureInPicture(
    content::WebContents* web_contents,
    const viz::SurfaceId& surface_id,
    const gfx::Size& natural_size) {
#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
  return PictureInPictureWindowManager::GetInstance()->EnterPictureInPicture(
      web_contents, surface_id, natural_size);
#else
  return content::PictureInPictureResult::kNotSupported;
#endif
}

void WebContents::ExitPictureInPicture() {
#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
  PictureInPictureWindowManager::GetInstance()->ExitPictureInPicture();
#endif
}

void WebContents::DevToolsSaveToFile(const std::string& url,
                                     const std::string& content,
                                     bool save_as) {
  base::FilePath path;
  auto it = saved_files_.find(url);
  if (it != saved_files_.end() && !save_as) {
    path = it->second;
  } else {
    file_dialog::DialogSettings settings;
    settings.parent_window = owner_window();
    settings.force_detached = offscreen_;
    settings.title = url;
    settings.default_path = base::FilePath::FromUTF8Unsafe(url);
    if (!file_dialog::ShowSaveDialogSync(settings, &path)) {
      base::Value url_value(url);
      inspectable_web_contents_->CallClientFunction(
          "DevToolsAPI.canceledSaveURL", &url_value, nullptr, nullptr);
      return;
    }
  }

  saved_files_[url] = path;
  // 通知DevTools。
  base::Value url_value(url);
  base::Value file_system_path_value(path.AsUTF8Unsafe());
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.savedURL", &url_value, &file_system_path_value, nullptr);
  file_task_runner_->PostTask(FROM_HERE,
                              base::BindOnce(&WriteToFile, path, content));
}

void WebContents::DevToolsAppendToFile(const std::string& url,
                                       const std::string& content) {
  auto it = saved_files_.find(url);
  if (it == saved_files_.end())
    return;

  // 通知DevTools。
  base::Value url_value(url);
  inspectable_web_contents_->CallClientFunction("DevToolsAPI.appendedToURL",
                                                &url_value, nullptr, nullptr);
  file_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AppendToFile, it->second, content));
}

void WebContents::DevToolsRequestFileSystems() {
  auto file_system_paths = GetAddedFileSystemPaths(GetDevToolsWebContents());
  if (file_system_paths.empty()) {
    base::ListValue empty_file_system_value;
    inspectable_web_contents_->CallClientFunction(
        "DevToolsAPI.fileSystemsLoaded", &empty_file_system_value, nullptr,
        nullptr);
    return;
  }

  std::vector<FileSystem> file_systems;
  for (const auto& file_system_path : file_system_paths) {
    base::FilePath path =
        base::FilePath::FromUTF8Unsafe(file_system_path.first);
    std::string file_system_id =
        RegisterFileSystem(GetDevToolsWebContents(), path);
    FileSystem file_system =
        CreateFileSystemStruct(GetDevToolsWebContents(), file_system_id,
                               file_system_path.first, file_system_path.second);
    file_systems.push_back(file_system);
  }

  base::ListValue file_system_value;
  for (const auto& file_system : file_systems)
    file_system_value.Append(CreateFileSystemValue(file_system));
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.fileSystemsLoaded", &file_system_value, nullptr, nullptr);
}

void WebContents::DevToolsAddFileSystem(
    const std::string& type,
    const base::FilePath& file_system_path) {
  base::FilePath path = file_system_path;
  if (path.empty()) {
    std::vector<base::FilePath> paths;
    file_dialog::DialogSettings settings;
    settings.parent_window = owner_window();
    settings.force_detached = offscreen_;
    settings.properties = file_dialog::OPEN_DIALOG_OPEN_DIRECTORY;
    if (!file_dialog::ShowOpenDialogSync(settings, &paths))
      return;

    path = paths[0];
  }

  std::string file_system_id =
      RegisterFileSystem(GetDevToolsWebContents(), path);
  if (IsDevToolsFileSystemAdded(GetDevToolsWebContents(), path.AsUTF8Unsafe()))
    return;

  FileSystem file_system = CreateFileSystemStruct(
      GetDevToolsWebContents(), file_system_id, path.AsUTF8Unsafe(), type);
  std::unique_ptr<base::DictionaryValue> file_system_value(
      CreateFileSystemValue(file_system));

  auto* pref_service = GetPrefService(GetDevToolsWebContents());
  DictionaryPrefUpdate update(pref_service, prefs::kDevToolsFileSystemPaths);
  update.Get()->SetWithoutPathExpansion(path.AsUTF8Unsafe(),
                                        std::make_unique<base::Value>(type));
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.fileSystemAdded", nullptr, file_system_value.get(), nullptr);
}

void WebContents::DevToolsRemoveFileSystem(
    const base::FilePath& file_system_path) {
  if (!inspectable_web_contents_)
    return;

  std::string path = file_system_path.AsUTF8Unsafe();
  storage::IsolatedContext::GetInstance()->RevokeFileSystemByPath(
      file_system_path);

  auto* pref_service = GetPrefService(GetDevToolsWebContents());
  DictionaryPrefUpdate update(pref_service, prefs::kDevToolsFileSystemPaths);
  update.Get()->RemoveKey(path);

  base::Value file_system_path_value(path);
  inspectable_web_contents_->CallClientFunction("DevToolsAPI.fileSystemRemoved",
                                                &file_system_path_value,
                                                nullptr, nullptr);
}

void WebContents::DevToolsIndexPath(
    int request_id,
    const std::string& file_system_path,
    const std::string& excluded_folders_message) {
  if (!IsDevToolsFileSystemAdded(GetDevToolsWebContents(), file_system_path)) {
    OnDevToolsIndexingDone(request_id, file_system_path);
    return;
  }
  if (devtools_indexing_jobs_.count(request_id) != 0)
    return;
  std::vector<std::string> excluded_folders;
  std::unique_ptr<base::Value> parsed_excluded_folders =
      base::JSONReader::ReadDeprecated(excluded_folders_message);
  if (parsed_excluded_folders && parsed_excluded_folders->is_list()) {
    for (const base::Value& folder_path : parsed_excluded_folders->GetList()) {
      if (folder_path.is_string())
        excluded_folders.push_back(folder_path.GetString());
    }
  }
  devtools_indexing_jobs_[request_id] =
      scoped_refptr<DevToolsFileSystemIndexer::FileSystemIndexingJob>(
          devtools_file_system_indexer_->IndexPath(
              file_system_path, excluded_folders,
              base::BindRepeating(
                  &WebContents::OnDevToolsIndexingWorkCalculated,
                  weak_factory_.GetWeakPtr(), request_id, file_system_path),
              base::BindRepeating(&WebContents::OnDevToolsIndexingWorked,
                                  weak_factory_.GetWeakPtr(), request_id,
                                  file_system_path),
              base::BindRepeating(&WebContents::OnDevToolsIndexingDone,
                                  weak_factory_.GetWeakPtr(), request_id,
                                  file_system_path)));
}

void WebContents::DevToolsStopIndexing(int request_id) {
  auto it = devtools_indexing_jobs_.find(request_id);
  if (it == devtools_indexing_jobs_.end())
    return;
  it->second->Stop();
  devtools_indexing_jobs_.erase(it);
}

void WebContents::DevToolsSearchInPath(int request_id,
                                       const std::string& file_system_path,
                                       const std::string& query) {
  if (!IsDevToolsFileSystemAdded(GetDevToolsWebContents(), file_system_path)) {
    OnDevToolsSearchCompleted(request_id, file_system_path,
                              std::vector<std::string>());
    return;
  }
  devtools_file_system_indexer_->SearchInPath(
      file_system_path, query,
      base::BindRepeating(&WebContents::OnDevToolsSearchCompleted,
                          weak_factory_.GetWeakPtr(), request_id,
                          file_system_path));
}

void WebContents::DevToolsSetEyeDropperActive(bool active) {
  auto* web_contents = GetWebContents();
  if (!web_contents)
    return;

  if (active) {
    eye_dropper_ = std::make_unique<DevToolsEyeDropper>(
        web_contents, base::BindRepeating(&WebContents::ColorPickedInEyeDropper,
                                          base::Unretained(this)));
  } else {
    eye_dropper_.reset();
  }
}

void WebContents::ColorPickedInEyeDropper(int r, int g, int b, int a) {
  base::DictionaryValue color;
  color.SetInteger("r", r);
  color.SetInteger("g", g);
  color.SetInteger("b", b);
  color.SetInteger("a", a);
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.eyeDropperPickedColor", &color, nullptr, nullptr);
}

#if defined(TOOLKIT_VIEWS) && !defined(OS_MAC)
ui::ImageModel WebContents::GetDevToolsWindowIcon() {
  return owner_window() ? owner_window()->GetWindowAppIcon() : ui::ImageModel{};
}
#endif

#if defined(OS_LINUX)
void WebContents::GetDevToolsWindowWMClass(std::string* name,
                                           std::string* class_name) {
  *class_name = Browser::Get()->GetName();
  *name = base::ToLowerASCII(*class_name);
}
#endif

void WebContents::OnDevToolsIndexingWorkCalculated(
    int request_id,
    const std::string& file_system_path,
    int total_work) {
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  base::Value total_work_value(total_work);
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.indexingTotalWorkCalculated", &request_id_value,
      &file_system_path_value, &total_work_value);
}

void WebContents::OnDevToolsIndexingWorked(int request_id,
                                           const std::string& file_system_path,
                                           int worked) {
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  base::Value worked_value(worked);
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.indexingWorked", &request_id_value, &file_system_path_value,
      &worked_value);
}

void WebContents::OnDevToolsIndexingDone(int request_id,
                                         const std::string& file_system_path) {
  devtools_indexing_jobs_.erase(request_id);
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.indexingDone", &request_id_value, &file_system_path_value,
      nullptr);
}

void WebContents::OnDevToolsSearchCompleted(
    int request_id,
    const std::string& file_system_path,
    const std::vector<std::string>& file_paths) {
  base::ListValue file_paths_value;
  for (const auto& file_path : file_paths) {
    file_paths_value.Append(file_path);
  }
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.searchCompleted", &request_id_value, &file_system_path_value,
      &file_paths_value);
}

void WebContents::SetHtmlApiFullscreen(bool enter_fullscreen) {
  // 窗口已处于全屏模式，请保存状态。
  if (enter_fullscreen && owner_window_->IsFullscreen()) {
    native_fullscreen_ = true;
    UpdateHtmlApiFullscreen(true);
    return;
  }

  // 退出html全屏状态，但不退出Windows的全屏模式。
  if (!enter_fullscreen && native_fullscreen_) {
    UpdateHtmlApiFullscreen(false);
    return;
  }

  // 如果允许，在窗口上设置全屏。
  auto* web_preferences = WebContentsPreferences::From(GetWebContents());
  bool html_fullscreenable =
      web_preferences
          ? !web_preferences->ShouldDisableHtmlFullscreenWindowResize()
          : true;

  if (html_fullscreenable) {
    owner_window_->SetFullScreen(enter_fullscreen);
  }

  UpdateHtmlApiFullscreen(enter_fullscreen);
  native_fullscreen_ = false;
}

void WebContents::UpdateHtmlApiFullscreen(bool fullscreen) {
  if (fullscreen == is_html_fullscreen())
    return;

  html_fullscreen_ = fullscreen;

  // 通知呈现器HTML全屏更改。
  web_contents()
      ->GetRenderViewHost()
      ->GetWidget()
      ->SynchronizeVisualProperties();

  // 嵌入器WebContents与WebView的框架树分离，因此。
  // 我们必须手动同步他们的全屏状态。
  if (embedder_)
    embedder_->SetHtmlApiFullscreen(fullscreen);

  if (fullscreen) {
    Emit("enter-html-full-screen");
    owner_window_->NotifyWindowEnterHtmlFullScreen();
  } else {
    Emit("leave-html-full-screen");
    owner_window_->NotifyWindowLeaveHtmlFullScreen();
  }

  // 确保所有子级网页浏览量退出html全屏。
  if (!fullscreen && !IsGuest()) {
    auto* manager = WebViewManager::GetWebViewManager(web_contents());
    manager->ForEachGuest(
        web_contents(), base::BindRepeating([](content::WebContents* guest) {
          WebContents* api_web_contents = WebContents::From(guest);
          api_web_contents->SetHtmlApiFullscreen(false);
          return false;
        }));
  }
}

// 静电。
v8::Local<v8::ObjectTemplate> WebContents::FillObjectTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  gin::InvokerOptions options;
  options.holder_is_first_argument = true;
  options.holder_type = "WebContents";
  templ->Set(
      gin::StringToSymbol(isolate, "isDestroyed"),
      gin::CreateFunctionTemplate(
          isolate, base::BindRepeating(&gin_helper::Destroyable::IsDestroyed),
          options));
  // 我们使用gin_helper：：ObjectTemplateBuilder代替。
  // GIN：：ObjectTemplateBuilder在这里处理WebContents是。
  // 可摧毁的。
  return gin_helper::ObjectTemplateBuilder(isolate, templ)
      .SetMethod("destroy", &WebContents::Destroy)
      .SetMethod("getBackgroundThrottling",
                 &WebContents::GetBackgroundThrottling)
      .SetMethod("setBackgroundThrottling",
                 &WebContents::SetBackgroundThrottling)
      .SetMethod("getProcessId", &WebContents::GetProcessID)
      .SetMethod("getOSProcessId", &WebContents::GetOSProcessID)
      .SetMethod("equal", &WebContents::Equal)
      .SetMethod("_loadURL", &WebContents::LoadURL)
      .SetMethod("reload", &WebContents::Reload)
      .SetMethod("reloadIgnoringCache", &WebContents::ReloadIgnoringCache)
      .SetMethod("downloadURL", &WebContents::DownloadURL)
      .SetMethod("getURL", &WebContents::GetURL)
      .SetMethod("getTitle", &WebContents::GetTitle)
      .SetMethod("isLoading", &WebContents::IsLoading)
      .SetMethod("isLoadingMainFrame", &WebContents::IsLoadingMainFrame)
      .SetMethod("isWaitingForResponse", &WebContents::IsWaitingForResponse)
      .SetMethod("stop", &WebContents::Stop)
      .SetMethod("canGoBack", &WebContents::CanGoBack)
      .SetMethod("goBack", &WebContents::GoBack)
      .SetMethod("canGoForward", &WebContents::CanGoForward)
      .SetMethod("goForward", &WebContents::GoForward)
      .SetMethod("canGoToOffset", &WebContents::CanGoToOffset)
      .SetMethod("goToOffset", &WebContents::GoToOffset)
      .SetMethod("canGoToIndex", &WebContents::CanGoToIndex)
      .SetMethod("goToIndex", &WebContents::GoToIndex)
      .SetMethod("getActiveIndex", &WebContents::GetActiveIndex)
      .SetMethod("clearHistory", &WebContents::ClearHistory)
      .SetMethod("length", &WebContents::GetHistoryLength)
      .SetMethod("isCrashed", &WebContents::IsCrashed)
      .SetMethod("forcefullyCrashRenderer",
                 &WebContents::ForcefullyCrashRenderer)
      .SetMethod("setUserAgent", &WebContents::SetUserAgent)
      .SetMethod("getUserAgent", &WebContents::GetUserAgent)
      .SetMethod("savePage", &WebContents::SavePage)
      .SetMethod("openDevTools", &WebContents::OpenDevTools)
      .SetMethod("closeDevTools", &WebContents::CloseDevTools)
      .SetMethod("isDevToolsOpened", &WebContents::IsDevToolsOpened)
      .SetMethod("isDevToolsFocused", &WebContents::IsDevToolsFocused)
      .SetMethod("enableDeviceEmulation", &WebContents::EnableDeviceEmulation)
      .SetMethod("disableDeviceEmulation", &WebContents::DisableDeviceEmulation)
      .SetMethod("toggleDevTools", &WebContents::ToggleDevTools)
      .SetMethod("inspectElement", &WebContents::InspectElement)
      .SetMethod("setIgnoreMenuShortcuts", &WebContents::SetIgnoreMenuShortcuts)
      .SetMethod("setAudioMuted", &WebContents::SetAudioMuted)
      .SetMethod("isAudioMuted", &WebContents::IsAudioMuted)
      .SetMethod("isCurrentlyAudible", &WebContents::IsCurrentlyAudible)
      .SetMethod("undo", &WebContents::Undo)
      .SetMethod("redo", &WebContents::Redo)
      .SetMethod("cut", &WebContents::Cut)
      .SetMethod("copy", &WebContents::Copy)
      .SetMethod("paste", &WebContents::Paste)
      .SetMethod("pasteAndMatchStyle", &WebContents::PasteAndMatchStyle)
      .SetMethod("delete", &WebContents::Delete)
      .SetMethod("selectAll", &WebContents::SelectAll)
      .SetMethod("unselect", &WebContents::Unselect)
      .SetMethod("replace", &WebContents::Replace)
      .SetMethod("replaceMisspelling", &WebContents::ReplaceMisspelling)
      .SetMethod("findInPage", &WebContents::FindInPage)
      .SetMethod("stopFindInPage", &WebContents::StopFindInPage)
      .SetMethod("focus", &WebContents::Focus)
      .SetMethod("isFocused", &WebContents::IsFocused)
      .SetMethod("sendInputEvent", &WebContents::SendInputEvent)
      .SetMethod("beginFrameSubscription", &WebContents::BeginFrameSubscription)
      .SetMethod("endFrameSubscription", &WebContents::EndFrameSubscription)
      .SetMethod("startDrag", &WebContents::StartDrag)
      .SetMethod("attachToIframe", &WebContents::AttachToIframe)
      .SetMethod("detachFromOuterFrame", &WebContents::DetachFromOuterFrame)
      .SetMethod("isOffscreen", &WebContents::IsOffScreen)
#if BUILDFLAG(ENABLE_OSR)
      .SetMethod("startPainting", &WebContents::StartPainting)
      .SetMethod("stopPainting", &WebContents::StopPainting)
      .SetMethod("isPainting", &WebContents::IsPainting)
      .SetMethod("setFrameRate", &WebContents::SetFrameRate)
      .SetMethod("getFrameRate", &WebContents::GetFrameRate)
#endif
      .SetMethod("invalidate", &WebContents::Invalidate)
      .SetMethod("setZoomLevel", &WebContents::SetZoomLevel)
      .SetMethod("getZoomLevel", &WebContents::GetZoomLevel)
      .SetMethod("setZoomFactor", &WebContents::SetZoomFactor)
      .SetMethod("getZoomFactor", &WebContents::GetZoomFactor)
      .SetMethod("getType", &WebContents::GetType)
      .SetMethod("_getPreloadPaths", &WebContents::GetPreloadPaths)
      .SetMethod("getLastWebPreferences", &WebContents::GetLastWebPreferences)
      .SetMethod("getOwnerBrowserWindow", &WebContents::GetOwnerBrowserWindow)
      .SetMethod("inspectServiceWorker", &WebContents::InspectServiceWorker)
      .SetMethod("inspectSharedWorker", &WebContents::InspectSharedWorker)
      .SetMethod("inspectSharedWorkerById",
                 &WebContents::InspectSharedWorkerById)
      .SetMethod("getAllSharedWorkers", &WebContents::GetAllSharedWorkers)
#if BUILDFLAG(ENABLE_PRINTING)
      .SetMethod("_print", &WebContents::Print)
      .SetMethod("_printToPDF", &WebContents::PrintToPDF)
#endif
      .SetMethod("_setNextChildWebPreferences",
                 &WebContents::SetNextChildWebPreferences)
      .SetMethod("addWorkSpace", &WebContents::AddWorkSpace)
      .SetMethod("removeWorkSpace", &WebContents::RemoveWorkSpace)
      .SetMethod("showDefinitionForSelection",
                 &WebContents::ShowDefinitionForSelection)
      .SetMethod("copyImageAt", &WebContents::CopyImageAt)
      .SetMethod("capturePage", &WebContents::CapturePage)
      .SetMethod("setEmbedder", &WebContents::SetEmbedder)
      .SetMethod("setDevToolsWebContents", &WebContents::SetDevToolsWebContents)
      .SetMethod("getNativeView", &WebContents::GetNativeView)
      .SetMethod("incrementCapturerCount", &WebContents::IncrementCapturerCount)
      .SetMethod("decrementCapturerCount", &WebContents::DecrementCapturerCount)
      .SetMethod("isBeingCaptured", &WebContents::IsBeingCaptured)
      .SetMethod("setWebRTCIPHandlingPolicy",
                 &WebContents::SetWebRTCIPHandlingPolicy)
      .SetMethod("getWebRTCIPHandlingPolicy",
                 &WebContents::GetWebRTCIPHandlingPolicy)
      .SetMethod("takeHeapSnapshot", &WebContents::TakeHeapSnapshot)
      .SetMethod("setImageAnimationPolicy",
                 &WebContents::SetImageAnimationPolicy)
      .SetMethod("_getProcessMemoryInfo", &WebContents::GetProcessMemoryInfo)
      .SetProperty("id", &WebContents::ID)
      .SetProperty("session", &WebContents::Session)
      .SetProperty("hostWebContents", &WebContents::HostWebContents)
      .SetProperty("devToolsWebContents", &WebContents::DevToolsWebContents)
      .SetProperty("debugger", &WebContents::Debugger)
      .SetProperty("mainFrame", &WebContents::MainFrame)
      .Build();
}

const char* WebContents::GetTypeName() {
  return "WebContents";
}

ElectronBrowserContext* WebContents::GetBrowserContext() const {
  return static_cast<ElectronBrowserContext*>(
      web_contents()->GetBrowserContext());
}

// 静电。
gin::Handle<WebContents> WebContents::New(
    v8::Isolate* isolate,
    const gin_helper::Dictionary& options) {
  gin::Handle<WebContents> handle =
      gin::CreateHandle(isolate, new WebContents(isolate, options));
  v8::TryCatch try_catch(isolate);
  gin_helper::CallMethod(isolate, handle.get(), "_init");
  if (try_catch.HasCaught()) {
    node::errors::TriggerUncaughtException(isolate, try_catch);
  }
  return handle;
}

// 静电。
gin::Handle<WebContents> WebContents::CreateAndTake(
    v8::Isolate* isolate,
    std::unique_ptr<content::WebContents> web_contents,
    Type type) {
  gin::Handle<WebContents> handle = gin::CreateHandle(
      isolate, new WebContents(isolate, std::move(web_contents), type));
  v8::TryCatch try_catch(isolate);
  gin_helper::CallMethod(isolate, handle.get(), "_init");
  if (try_catch.HasCaught()) {
    node::errors::TriggerUncaughtException(isolate, try_catch);
  }
  return handle;
}

// 静电。
WebContents* WebContents::From(content::WebContents* web_contents) {
  if (!web_contents)
    return nullptr;
  auto* data = static_cast<UserDataLink*>(
      web_contents->GetUserData(kElectronApiWebContentsKey));
  return data ? data->web_contents.get() : nullptr;
}

// 静电。
gin::Handle<WebContents> WebContents::FromOrCreate(
    v8::Isolate* isolate,
    content::WebContents* web_contents) {
  WebContents* api_web_contents = From(web_contents);
  if (!api_web_contents) {
    api_web_contents = new WebContents(isolate, web_contents);
    v8::TryCatch try_catch(isolate);
    gin_helper::CallMethod(isolate, api_web_contents, "_init");
    if (try_catch.HasCaught()) {
      node::errors::TriggerUncaughtException(isolate, try_catch);
    }
  }
  return gin::CreateHandle(isolate, api_web_contents);
}

// 静电。
gin::Handle<WebContents> WebContents::CreateFromWebPreferences(
    v8::Isolate* isolate,
    const gin_helper::Dictionary& web_preferences) {
  // 检查webPreferences是否具有|webContents|选项。
  gin::Handle<WebContents> web_contents;
  if (web_preferences.GetHidden("webContents", &web_contents) &&
      !web_contents.IsEmpty()) {
    // 如果使用现有的webContents，请从选项设置webPreferences。
    // 这些首选项将在Web内容启动新内容时使用。
    // 渲染进程。
    auto* existing_preferences =
        WebContentsPreferences::From(web_contents->web_contents());
    gin_helper::Dictionary web_preferences_dict;
    if (gin::ConvertFromV8(isolate, web_preferences.GetHandle(),
                           &web_preferences_dict)) {
      existing_preferences->SetFromDictionary(web_preferences_dict);
      absl::optional<SkColor> color =
          existing_preferences->GetBackgroundColor();
      web_contents->web_contents()->SetPageBaseBackgroundColor(color);
      auto* rwhv = web_contents->web_contents()->GetRenderWidgetHostView();
      if (rwhv)
        rwhv->SetBackgroundColor(color.value_or(SK_ColorWHITE));
    }
  } else {
    // 如果没有，就创建一个。
    web_contents = WebContents::New(isolate, web_preferences);
  }

  return web_contents;
}

// 静电
WebContents* WebContents::FromID(int32_t id) {
  return GetAllWebContents().Lookup(id);
}

// 静电。
gin::WrapperInfo WebContents::kWrapperInfo = {gin::kEmbedderNativeGin};

}  // 命名空间API。

}  // 命名空间电子。

namespace {

using electron::api::GetAllWebContents;
using electron::api::WebContents;

gin::Handle<WebContents> WebContentsFromID(v8::Isolate* isolate, int32_t id) {
  WebContents* contents = WebContents::FromID(id);
  return contents ? gin::CreateHandle(isolate, contents)
                  : gin::Handle<WebContents>();
}

gin::Handle<WebContents> WebContentsFromDevToolsTargetID(
    v8::Isolate* isolate,
    std::string target_id) {
  auto agent_host = content::DevToolsAgentHost::GetForId(target_id);
  WebContents* contents =
      agent_host ? WebContents::From(agent_host->GetWebContents()) : nullptr;
  return contents ? gin::CreateHandle(isolate, contents)
                  : gin::Handle<WebContents>();
}

std::vector<gin::Handle<WebContents>> GetAllWebContentsAsV8(
    v8::Isolate* isolate) {
  std::vector<gin::Handle<WebContents>> list;
  for (auto iter = base::IDMap<WebContents*>::iterator(&GetAllWebContents());
       !iter.IsAtEnd(); iter.Advance()) {
    list.push_back(gin::CreateHandle(isolate, iter.GetCurrentValue()));
  }
  return list;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("WebContents", WebContents::GetConstructor(context));
  dict.SetMethod("fromId", &WebContentsFromID);
  dict.SetMethod("fromDevToolsTargetId", &WebContentsFromDevToolsTargetID);
  dict.SetMethod("getAllWebContents", &GetAllWebContentsAsV8);
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_web_contents, Initialize)
