// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_auto_updater.h"

#include "base/time/time.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/time_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

gin::WrapperInfo AutoUpdater::kWrapperInfo = {gin::kEmbedderNativeGin};

AutoUpdater::AutoUpdater() {
  auto_updater::AutoUpdater::SetDelegate(this);
}

AutoUpdater::~AutoUpdater() {
  auto_updater::AutoUpdater::SetDelegate(nullptr);
}

void AutoUpdater::OnError(const std::string& message) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (GetWrapper(isolate).ToLocal(&wrapper)) {
    auto error = v8::Exception::Error(gin::StringToV8(isolate, message));
    gin_helper::EmitEvent(
        isolate, wrapper, "error",
        error->ToObject(isolate->GetCurrentContext()).ToLocalChecked(),
        // 还会发出消息以保持与旧代码的兼容性。
        message);
  }
}

void AutoUpdater::OnError(const std::string& message,
                          const int code,
                          const std::string& domain) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (GetWrapper(isolate).ToLocal(&wrapper)) {
    auto error = v8::Exception::Error(gin::StringToV8(isolate, message));
    auto errorObject =
        error->ToObject(isolate->GetCurrentContext()).ToLocalChecked();

    auto context = isolate->GetCurrentContext();

    // 添加两个新参数以更好地处理错误。
    errorObject
        ->Set(context, gin::StringToV8(isolate, "code"),
              v8::Integer::New(isolate, code))
        .Check();
    errorObject
        ->Set(context, gin::StringToV8(isolate, "domain"),
              gin::StringToV8(isolate, domain))
        .Check();

    gin_helper::EmitEvent(isolate, wrapper, "error", errorObject, message);
  }
}

void AutoUpdater::OnCheckingForUpdate() {
  Emit("checking-for-update");
}

void AutoUpdater::OnUpdateAvailable() {
  Emit("update-available");
}

void AutoUpdater::OnUpdateNotAvailable() {
  Emit("update-not-available");
}

void AutoUpdater::OnUpdateDownloaded(const std::string& release_notes,
                                     const std::string& release_name,
                                     const base::Time& release_date,
                                     const std::string& url) {
  Emit("update-downloaded", release_notes, release_name, release_date, url,
       // 保持与旧API的兼容性。
       base::BindRepeating(&AutoUpdater::QuitAndInstall,
                           base::Unretained(this)));
}

void AutoUpdater::OnWindowAllClosed() {
  QuitAndInstall();
}

void AutoUpdater::SetFeedURL(gin::Arguments* args) {
  auto_updater::AutoUpdater::SetFeedURL(args);
}

void AutoUpdater::QuitAndInstall() {
  Emit("before-quit-for-update");

  // 如果我们没有任何窗口，则立即退出并安装。
  if (WindowList::IsEmpty()) {
    auto_updater::AutoUpdater::QuitAndInstall();
    return;
  }

  // 否则，请在关闭所有窗口后重新启动。
  WindowList::AddObserver(this);
  WindowList::CloseAllWindows();
}

// 静电。
gin::Handle<AutoUpdater> AutoUpdater::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new AutoUpdater());
}

gin::ObjectTemplateBuilder AutoUpdater::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<AutoUpdater>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("checkForUpdates", &auto_updater::AutoUpdater::CheckForUpdates)
      .SetMethod("getFeedURL", &auto_updater::AutoUpdater::GetFeedURL)
      .SetMethod("setFeedURL", &AutoUpdater::SetFeedURL)
      .SetMethod("quitAndInstall", &AutoUpdater::QuitAndInstall);
}

const char* AutoUpdater::GetTypeName() {
  return "AutoUpdater";
}

}  // 命名空间API。

}  // 命名空间电子。

namespace {

using electron::api::AutoUpdater;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("autoUpdater", AutoUpdater::Create(isolate));
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_auto_updater, Initialize)
