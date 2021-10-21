// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_download_item.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/filename_util.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/common/gin_converters/file_dialog_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "url/gurl.h"

namespace gin {

template <>
struct Converter<download::DownloadItem::DownloadState> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      download::DownloadItem::DownloadState state) {
    std::string download_state;
    switch (state) {
      case download::DownloadItem::IN_PROGRESS:
        download_state = "progressing";
        break;
      case download::DownloadItem::COMPLETE:
        download_state = "completed";
        break;
      case download::DownloadItem::CANCELLED:
        download_state = "cancelled";
        break;
      case download::DownloadItem::INTERRUPTED:
        download_state = "interrupted";
        break;
      default:
        break;
    }
    return ConvertToV8(isolate, download_state);
  }
};

}  // 命名空间杜松子酒。

namespace electron {

namespace api {

namespace {

// 通常base：：SupportsUserData只支持强链接，其中。
// 用户数据附加到的事物拥有用户数据。但是我们不能。
// 使API：：DownloadItem归DownloadItem所有，因为它由。
// V8。所以这就是一个薄弱环节。DownloadItem和DownloadItem的生存期。
// API：：DownloadItem是完全独立的，任何一个都可能被销毁。
// 在另一个之前。
struct UserDataLink : base::SupportsUserData::Data {
  explicit UserDataLink(base::WeakPtr<DownloadItem> item)
      : download_item(item) {}

  base::WeakPtr<DownloadItem> download_item;
};

const void* kElectronApiDownloadItemKey = &kElectronApiDownloadItemKey;

}  // 命名空间。

gin::WrapperInfo DownloadItem::kWrapperInfo = {gin::kEmbedderNativeGin};

// 静电。
DownloadItem* DownloadItem::FromDownloadItem(download::DownloadItem* item) {
  // ^-连续说出7倍的速度。
  auto* data = static_cast<UserDataLink*>(
      item->GetUserData(kElectronApiDownloadItemKey));
  return data ? data->download_item.get() : nullptr;
}

DownloadItem::DownloadItem(v8::Isolate* isolate, download::DownloadItem* item)
    : download_item_(item), isolate_(isolate) {
  download_item_->AddObserver(this);
  download_item_->SetUserData(
      kElectronApiDownloadItemKey,
      std::make_unique<UserDataLink>(weak_factory_.GetWeakPtr()));
}

DownloadItem::~DownloadItem() {
  if (download_item_) {
    // 被垃圾收集或销毁()销毁。
    download_item_->RemoveObserver(this);
    download_item_->Remove();
  }
}

bool DownloadItem::CheckAlive() const {
  if (!download_item_) {
    gin_helper::ErrorThrower(isolate_).ThrowError(
        "DownloadItem used after being destroyed");
    return false;
  }
  return true;
}

void DownloadItem::OnDownloadUpdated(download::DownloadItem* item) {
  if (!CheckAlive())
    return;
  if (download_item_->IsDone()) {
    Emit("done", item->GetState());
    Unpin();
  } else {
    Emit("updated", item->GetState());
  }
}

void DownloadItem::OnDownloadDestroyed(download::DownloadItem* /* 项目。*/) {
  download_item_ = nullptr;
  Unpin();
}

void DownloadItem::Pause() {
  if (!CheckAlive())
    return;
  download_item_->Pause();
}

bool DownloadItem::IsPaused() const {
  if (!CheckAlive())
    return false;
  return download_item_->IsPaused();
}

void DownloadItem::Resume() {
  if (!CheckAlive())
    return;
  download_item_->Resume(true /* 用户手势(_S)。*/);
}

bool DownloadItem::CanResume() const {
  if (!CheckAlive())
    return false;
  return download_item_->CanResume();
}

void DownloadItem::Cancel() {
  if (!CheckAlive())
    return;
  download_item_->Cancel(true);
}

int64_t DownloadItem::GetReceivedBytes() const {
  if (!CheckAlive())
    return 0;
  return download_item_->GetReceivedBytes();
}

int64_t DownloadItem::GetTotalBytes() const {
  if (!CheckAlive())
    return 0;
  return download_item_->GetTotalBytes();
}

std::string DownloadItem::GetMimeType() const {
  if (!CheckAlive())
    return "";
  return download_item_->GetMimeType();
}

bool DownloadItem::HasUserGesture() const {
  if (!CheckAlive())
    return false;
  return download_item_->HasUserGesture();
}

std::string DownloadItem::GetFilename() const {
  if (!CheckAlive())
    return "";
  return base::UTF16ToUTF8(
      net::GenerateFileName(GetURL(), GetContentDisposition(), std::string(),
                            download_item_->GetSuggestedFilename(),
                            GetMimeType(), "download")
          .LossyDisplayName());
}

std::string DownloadItem::GetContentDisposition() const {
  if (!CheckAlive())
    return "";
  return download_item_->GetContentDisposition();
}

const GURL& DownloadItem::GetURL() const {
  if (!CheckAlive())
    return GURL::EmptyGURL();
  return download_item_->GetURL();
}

v8::Local<v8::Value> DownloadItem::GetURLChain() const {
  if (!CheckAlive())
    return v8::Local<v8::Value>();
  return gin::ConvertToV8(isolate_, download_item_->GetUrlChain());
}

download::DownloadItem::DownloadState DownloadItem::GetState() const {
  if (!CheckAlive())
    return download::DownloadItem::IN_PROGRESS;
  return download_item_->GetState();
}

bool DownloadItem::IsDone() const {
  if (!CheckAlive())
    return false;
  return download_item_->IsDone();
}

void DownloadItem::SetSavePath(const base::FilePath& path) {
  save_path_ = path;
}

base::FilePath DownloadItem::GetSavePath() const {
  return save_path_;
}

file_dialog::DialogSettings DownloadItem::GetSaveDialogOptions() const {
  return dialog_options_;
}

void DownloadItem::SetSaveDialogOptions(
    const file_dialog::DialogSettings& options) {
  dialog_options_ = options;
}

std::string DownloadItem::GetLastModifiedTime() const {
  if (!CheckAlive())
    return "";
  return download_item_->GetLastModifiedTime();
}

std::string DownloadItem::GetETag() const {
  if (!CheckAlive())
    return "";
  return download_item_->GetETag();
}

double DownloadItem::GetStartTime() const {
  if (!CheckAlive())
    return 0;
  return download_item_->GetStartTime().ToDoubleT();
}

// 静电。
gin::ObjectTemplateBuilder DownloadItem::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<DownloadItem>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("pause", &DownloadItem::Pause)
      .SetMethod("isPaused", &DownloadItem::IsPaused)
      .SetMethod("resume", &DownloadItem::Resume)
      .SetMethod("canResume", &DownloadItem::CanResume)
      .SetMethod("cancel", &DownloadItem::Cancel)
      .SetMethod("getReceivedBytes", &DownloadItem::GetReceivedBytes)
      .SetMethod("getTotalBytes", &DownloadItem::GetTotalBytes)
      .SetMethod("getMimeType", &DownloadItem::GetMimeType)
      .SetMethod("hasUserGesture", &DownloadItem::HasUserGesture)
      .SetMethod("getFilename", &DownloadItem::GetFilename)
      .SetMethod("getContentDisposition", &DownloadItem::GetContentDisposition)
      .SetMethod("getURL", &DownloadItem::GetURL)
      .SetMethod("getURLChain", &DownloadItem::GetURLChain)
      .SetMethod("getState", &DownloadItem::GetState)
      .SetMethod("isDone", &DownloadItem::IsDone)
      .SetMethod("setSavePath", &DownloadItem::SetSavePath)
      .SetMethod("getSavePath", &DownloadItem::GetSavePath)
      .SetProperty("savePath", &DownloadItem::GetSavePath,
                   &DownloadItem::SetSavePath)
      .SetMethod("setSaveDialogOptions", &DownloadItem::SetSaveDialogOptions)
      .SetMethod("getSaveDialogOptions", &DownloadItem::GetSaveDialogOptions)
      .SetMethod("getLastModifiedTime", &DownloadItem::GetLastModifiedTime)
      .SetMethod("getETag", &DownloadItem::GetETag)
      .SetMethod("getStartTime", &DownloadItem::GetStartTime);
}

const char* DownloadItem::GetTypeName() {
  return "DownloadItem";
}

// 静电。
gin::Handle<DownloadItem> DownloadItem::FromOrCreate(
    v8::Isolate* isolate,
    download::DownloadItem* item) {
  DownloadItem* existing = FromDownloadItem(item);
  if (existing)
    return gin::CreateHandle(isolate, existing);

  auto handle = gin::CreateHandle(isolate, new DownloadItem(isolate, item));

  handle->Pin(isolate);

  return handle;
}

}  // 命名空间API。

}  // 命名空间电子
