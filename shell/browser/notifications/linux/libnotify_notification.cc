// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/notifications/linux/libnotify_notification.h"

#include <set>
#include <string>

#include "base/files/file_enumerator.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/ui/gtk_util.h"
#include "shell/common/application_info.h"
#include "shell/common/platform_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gtk/gtk_util.h"

namespace electron {

namespace {

LibNotifyLoader libnotify_loader_;

const std::set<std::string>& GetServerCapabilities() {
  static std::set<std::string> caps;
  if (caps.empty()) {
    auto* capabilities = libnotify_loader_.notify_get_server_caps();
    for (auto* l = capabilities; l != nullptr; l = l->next)
      caps.insert(static_cast<const char*>(l->data));
    g_list_free_full(capabilities, g_free);
  }
  return caps;
}

bool HasCapability(const std::string& capability) {
  return GetServerCapabilities().count(capability) != 0;
}

bool NotifierSupportsActions() {
  if (getenv("ELECTRON_USE_UBUNTU_NOTIFIER"))
    return false;

  return HasCapability("actions");
}

void log_and_clear_error(GError* error, const char* context) {
  LOG(ERROR) << context << ": domain=" << error->domain
             << " code=" << error->code << " message=\"" << error->message
             << '"';
  g_error_free(error);
}

}  // 命名空间。

// 静电。
bool LibnotifyNotification::Initialize() {
  if (!libnotify_loader_.Load("libnotify.so.4") &&  // 最常见的一种。
      !libnotify_loader_.Load("libnotify.so.5") &&
      !libnotify_loader_.Load("libnotify.so.1") &&
      !libnotify_loader_.Load("libnotify.so")) {
    LOG(WARNING) << "Unable to find libnotify; notifications disabled";
    return false;
  }
  if (!libnotify_loader_.notify_is_initted() &&
      !libnotify_loader_.notify_init(GetApplicationName().c_str())) {
    LOG(WARNING) << "Unable to initialize libnotify; notifications disabled";
    return false;
  }
  return true;
}

LibnotifyNotification::LibnotifyNotification(NotificationDelegate* delegate,
                                             NotificationPresenter* presenter)
    : Notification(delegate, presenter) {}

LibnotifyNotification::~LibnotifyNotification() {
  if (notification_) {
    g_signal_handlers_disconnect_by_data(notification_, this);
    g_object_unref(notification_);
  }
}

void LibnotifyNotification::Show(const NotificationOptions& options) {
  notification_ = libnotify_loader_.notify_notification_new(
      base::UTF16ToUTF8(options.title).c_str(),
      base::UTF16ToUTF8(options.msg).c_str(), nullptr);

  g_signal_connect(notification_, "closed",
                   G_CALLBACK(OnNotificationClosedThunk), this);

  // 注：在Unity和使用Notify-OSD的任何其他DE上，添加通知。
  // 操作将使通知显示为模式对话框。
  if (NotifierSupportsActions()) {
    libnotify_loader_.notify_notification_add_action(
        notification_, "default", "View", OnNotificationViewThunk, this,
        nullptr);
  }

  NotifyUrgency urgency = NOTIFY_URGENCY_NORMAL;
  if (options.urgency == u"critical") {
    urgency = NOTIFY_URGENCY_CRITICAL;
  } else if (options.urgency == u"low") {
    urgency = NOTIFY_URGENCY_LOW;
  }

  // 设置通知的紧急级别。
  libnotify_loader_.notify_notification_set_urgency(notification_, urgency);

  if (!options.icon.drawsNothing()) {
    GdkPixbuf* pixbuf = gtk_util::GdkPixbufFromSkBitmap(options.icon);
    libnotify_loader_.notify_notification_set_image_from_pixbuf(notification_,
                                                                pixbuf);
    g_object_unref(pixbuf);
  }

  // 设置通知的超时持续时间。
  bool neverTimeout = options.timeout_type == u"never";
  int timeout = (neverTimeout) ? NOTIFY_EXPIRES_NEVER : NOTIFY_EXPIRES_DEFAULT;
  libnotify_loader_.notify_notification_set_timeout(notification_, timeout);

  if (!options.tag.empty()) {
    GQuark id = g_quark_from_string(options.tag.c_str());
    g_object_set(G_OBJECT(notification_), "id", id, NULL);
  }

  // 始终尝试附加通知。
  // 可以使用独特的标签来防止这种情况。
  if (HasCapability("append")) {
    libnotify_loader_.notify_notification_set_hint_string(notification_,
                                                          "append", "true");
  } else if (HasCapability("x-canonical-append")) {
    libnotify_loader_.notify_notification_set_hint_string(
        notification_, "x-canonical-append", "true");
  }

  // 发送桌面名称以标识应用程序。
  // Ktop-entry是.ktop之前的部分。
  std::string desktop_id;
  if (platform_util::GetDesktopName(&desktop_id)) {
    const std::string suffix{".desktop"};
    if (base::EndsWith(desktop_id, suffix,
                       base::CompareCase::INSENSITIVE_ASCII)) {
      desktop_id.resize(desktop_id.size() - suffix.size());
    }
    libnotify_loader_.notify_notification_set_hint_string(
        notification_, "desktop-entry", desktop_id.c_str());
  }

  GError* error = nullptr;
  libnotify_loader_.notify_notification_show(notification_, &error);
  if (error) {
    log_and_clear_error(error, "notify_notification_show");
    NotificationFailed();
    return;
  }

  if (delegate())
    delegate()->NotificationDisplayed();
}

void LibnotifyNotification::Dismiss() {
  if (!notification_) {
    Destroy();
    return;
  }

  GError* error = nullptr;
  libnotify_loader_.notify_notification_close(notification_, &error);
  if (error) {
    log_and_clear_error(error, "notify_notification_close");
    Destroy();
  }
}

void LibnotifyNotification::OnNotificationClosed(
    NotifyNotification* notification) {
  NotificationDismissed();
}

void LibnotifyNotification::OnNotificationView(NotifyNotification* notification,
                                               char* action) {
  NotificationClicked();
}

}  // 命名空间电子
