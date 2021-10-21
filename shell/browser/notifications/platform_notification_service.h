// 版权所有(C)2015年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_H_
#define SHELL_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_H_

#include <string>

#include "content/public/browser/platform_notification_service.h"

namespace electron {

class ElectronBrowserClient;

class PlatformNotificationService
    : public content::PlatformNotificationService {
 public:
  explicit PlatformNotificationService(ElectronBrowserClient* browser_client);
  ~PlatformNotificationService() override;

 protected:
  // 内容：：PlatformNotificationService：
  void DisplayNotification(
      content::RenderFrameHost* render_frame_host,
      const std::string& notification_id,
      const GURL& origin,
      const GURL& document_url,
      const blink::PlatformNotificationData& notification_data,
      const blink::NotificationResources& notification_resources) override;
  void DisplayPersistentNotification(
      const std::string& notification_id,
      const GURL& service_worker_scope,
      const GURL& origin,
      const blink::PlatformNotificationData& notification_data,
      const blink::NotificationResources& notification_resources) override;
  void ClosePersistentNotification(const std::string& notification_id) override;
  void CloseNotification(const std::string& notification_id) override;
  void GetDisplayedNotifications(
      DisplayedNotificationsCallback callback) override;
  int64_t ReadNextPersistentNotificationId() override;
  void RecordNotificationUkmEvent(
      const content::NotificationDatabaseData& data) override;
  void ScheduleTrigger(base::Time timestamp) override;
  base::Time ReadNextTriggerTimestamp() override;

 private:
  ElectronBrowserClient* browser_client_;

  DISALLOW_COPY_AND_ASSIGN(PlatformNotificationService);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NOTIFICATIONS_PLATFORM_NOTIFICATION_SERVICE_H_
