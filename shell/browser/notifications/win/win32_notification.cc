// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "shell/browser/notifications/win/win32_notification.h"

#include <windows.h>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace electron {

void Win32Notification::Show(const NotificationOptions& options) {
  auto* presenter = static_cast<NotificationPresenterWin7*>(this->presenter());
  if (!presenter)
    return;

  HBITMAP image = NULL;

  if (!options.icon.drawsNothing()) {
    if (options.icon.colorType() == kBGRA_8888_SkColorType) {
      BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
      bmi.biWidth = options.icon.width();
      bmi.biHeight = -options.icon.height();
      bmi.biPlanes = 1;
      bmi.biBitCount = 32;
      bmi.biCompression = BI_RGB;

      HDC hdcScreen = GetDC(NULL);
      image =
          CreateDIBitmap(hdcScreen, &bmi, CBM_INIT, options.icon.getPixels(),
                         reinterpret_cast<BITMAPINFO*>(&bmi), DIB_RGB_COLORS);
      ReleaseDC(NULL, hdcScreen);
    }
  }

  Win32Notification* existing = nullptr;
  if (!options.tag.empty())
    existing = presenter->GetNotificationObjectByTag(options.tag);

  if (existing) {
    existing->tag_.clear();

    this->notification_ref_ = std::move(existing->notification_ref_);
    this->notification_ref_.Set(options.title, options.msg, image);
    // 需要删除通知集中的条目。
    // NotificationPresenter正在等待。
    existing->Destroy();
  } else {
    this->notification_ref_ =
        presenter->AddNotification(options.title, options.msg, image);
  }

  this->tag_ = options.tag;

  if (image)
    DeleteObject(image);
}

void Win32Notification::Dismiss() {
  notification_ref_.Close();
}

}  // 命名空间电子
