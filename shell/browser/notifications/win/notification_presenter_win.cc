// 版权所有(C)2012 Chromium作者。版权所有。
// 版权所有(C)2015 Felix Rieseberg&lt;feriese@microsoft.com&gt;和。
// Jason Poon&lt;jason.poon@microsoft.com&gt;。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#include "shell/browser/notifications/win/notification_presenter_win.h"

#include <memory>
#include <string>
#include <vector>

#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/hash/md5.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/win/windows_version.h"
#include "shell/browser/notifications/win/notification_presenter_win7.h"
#include "shell/browser/notifications/win/windows_toast_notification.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"

#pragma comment(lib, "runtimeobject.lib")

namespace electron {

namespace {

bool IsDebuggingNotifications() {
  return base::Environment::Create()->HasVar("ELECTRON_DEBUG_NOTIFICATIONS");
}

bool SaveIconToPath(const SkBitmap& bitmap, const base::FilePath& path) {
  std::vector<unsigned char> png_data;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false, &png_data))
    return false;

  char* data = reinterpret_cast<char*>(&png_data[0]);
  int size = static_cast<int>(png_data.size());
  return base::WriteFile(path, data, size) == size;
}

}  // 命名空间。

// 静电。
NotificationPresenter* NotificationPresenter::Create() {
  auto version = base::win::GetVersion();
  if (version < base::win::Version::WIN8)
    return new NotificationPresenterWin7;
  if (!WindowsToastNotification::Initialize())
    return nullptr;
  auto presenter = std::make_unique<NotificationPresenterWin>();
  if (!presenter->Init())
    return nullptr;

  if (IsDebuggingNotifications())
    LOG(INFO) << "Successfully created Windows notifications presenter";

  return presenter.release();
}

NotificationPresenterWin::NotificationPresenterWin() = default;

NotificationPresenterWin::~NotificationPresenterWin() = default;

bool NotificationPresenterWin::Init() {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  return temp_dir_.CreateUniqueTempDir();
}

std::wstring NotificationPresenterWin::SaveIconToFilesystem(
    const SkBitmap& icon,
    const GURL& origin) {
  std::string filename;

  if (origin.is_valid()) {
    filename = base::MD5String(origin.spec()) + ".png";
  } else {
    base::TimeTicks now = base::TimeTicks::Now();
    filename = std::to_string(now.ToInternalValue()) + ".png";
  }

  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::FilePath path = temp_dir_.GetPath().Append(base::UTF8ToWide(filename));
  if (base::PathExists(path))
    return path.value();
  if (SaveIconToPath(icon, path))
    return path.value();
  return base::UTF8ToWide(origin.spec());
}

Notification* NotificationPresenterWin::CreateNotificationObject(
    NotificationDelegate* delegate) {
  return new WindowsToastNotification(delegate, this);
}

}  // 命名空间电子
