// 版权所有(C)2015年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#include "shell/browser/notifications/platform_notification_service.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/notification_event_dispatcher.h"
#include "content/public/browser/render_process_host.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/notifications/notification.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"
#include "third_party/blink/public/common/notifications/notification_resources.h"
#include "third_party/blink/public/common/notifications/platform_notification_data.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace electron {

namespace {

void OnWebNotificationAllowed(base::WeakPtr<Notification> notification,
                              const SkBitmap& icon,
                              const blink::PlatformNotificationData& data,
                              bool audio_muted,
                              bool allowed) {
  if (!notification)
    return;
  if (allowed) {
    electron::NotificationOptions options;
    options.title = data.title;
    options.msg = data.body;
    options.tag = data.tag;
    options.icon_url = data.icon;
    options.icon = icon;
    options.silent = audio_muted ? true : data.silent;
    options.has_reply = false;
    notification->Show(options);
  } else {
    notification->Destroy();
  }
}

class NotificationDelegateImpl final : public electron::NotificationDelegate {
 public:
  explicit NotificationDelegateImpl(const std::string& notification_id)
      : notification_id_(notification_id) {}

  void NotificationDestroyed() override { delete this; }

  void NotificationClick() override {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNonPersistentClickEvent(notification_id_, base::DoNothing());
  }

  void NotificationClosed() override {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNonPersistentCloseEvent(notification_id_, base::DoNothing());
  }

  void NotificationDisplayed() override {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNonPersistentShowEvent(notification_id_);
  }

 private:
  std::string notification_id_;

  DISALLOW_COPY_AND_ASSIGN(NotificationDelegateImpl);
};

}  // 命名空间。

PlatformNotificationService::PlatformNotificationService(
    ElectronBrowserClient* browser_client)
    : browser_client_(browser_client) {}

PlatformNotificationService::~PlatformNotificationService() = default;

void PlatformNotificationService::DisplayNotification(
    content::RenderFrameHost* render_frame_host,
    const std::string& notification_id,
    const GURL& origin,
    const GURL& document_url,
    const blink::PlatformNotificationData& notification_data,
    const blink::NotificationResources& notification_resources) {
  auto* presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;

  // 如果创建的新通知的标签与。
  // 现有通知，请用新通知替换旧通知。
  // NOTIFICATION_ID是从标记生成的，因此。
  // 通知将因此呼叫而关闭，如果存在以下情况。
  // 相同的标签已经存在。
  // 
  // 请参阅：https://notifications.spec.whatwg.org/#showing-a-notification。
  presenter->CloseNotificationWithId(notification_id);

  auto* delegate = new NotificationDelegateImpl(notification_id);

  auto notification = presenter->CreateNotification(delegate, notification_id);
  if (notification) {
    browser_client_->WebNotificationAllowed(
        render_frame_host,
        base::BindRepeating(&OnWebNotificationAllowed, notification,
                            notification_resources.notification_icon,
                            notification_data));
  }
}

void PlatformNotificationService::DisplayPersistentNotification(
    const std::string& notification_id,
    const GURL& service_worker_scope,
    const GURL& origin,
    const blink::PlatformNotificationData& notification_data,
    const blink::NotificationResources& notification_resources) {}

void PlatformNotificationService::ClosePersistentNotification(
    const std::string& notification_id) {}

void PlatformNotificationService::CloseNotification(
    const std::string& notification_id) {
  auto* presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;
  presenter->CloseNotificationWithId(notification_id);
}

void PlatformNotificationService::GetDisplayedNotifications(
    DisplayedNotificationsCallback callback) {}

int64_t PlatformNotificationService::ReadNextPersistentNotificationId() {
  // 电子邮件不支持持久通知。
  return 0;
}

void PlatformNotificationService::RecordNotificationUkmEvent(
    const content::NotificationDatabaseData& data) {}

void PlatformNotificationService::ScheduleTrigger(base::Time timestamp) {}

base::Time PlatformNotificationService::ReadNextTriggerTimestamp() {
  return base::Time::Max();
}

}  // 命名空间电子
