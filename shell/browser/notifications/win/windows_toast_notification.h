// 版权所有(C)2015年Felix Rieseberg&lt;feriese@microsoft.com&gt;和Jason Poon。
// &lt;jason.poon@microsoft.com&gt;。版权所有。
// 版权所有(C)2015 Ryan McShane&lt;rmcshane@Bandwidth.com&gt;和Brandon Smith。
// &lt;bsmith@Bandwidth.com&gt;。
// 感谢上面提到的两个人，他们最先想出了一堆。
// 此代码。
// 并以麻省理工学院的名义向全世界发布。

#ifndef SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_NOTIFICATION_H_
#define SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_NOTIFICATION_H_

#include <windows.h>
#include <windows.ui.notifications.h>
#include <wrl/implements.h>
#include <string>
#include <vector>

#include "shell/browser/notifications/notification.h"

using Microsoft::WRL::ClassicCom;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Make;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;

namespace electron {

class ScopedHString;

using DesktopToastActivatedEventHandler =
    ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification*,
        IInspectable*>;
using DesktopToastDismissedEventHandler =
    ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification*,
        ABI::Windows::UI::Notifications::ToastDismissedEventArgs*>;
using DesktopToastFailedEventHandler =
    ABI::Windows::Foundation::ITypedEventHandler<
        ABI::Windows::UI::Notifications::ToastNotification*,
        ABI::Windows::UI::Notifications::ToastFailedEventArgs*>;

class WindowsToastNotification : public Notification {
 public:
  // 只能由NotificationPresenterWin调用。
  static bool Initialize();

  WindowsToastNotification(NotificationDelegate* delegate,
                           NotificationPresenter* presenter);
  ~WindowsToastNotification() override;

 protected:
  // 通知：
  void Show(const NotificationOptions& options) override;
  void Dismiss() override;

 private:
  friend class ToastEventHandler;

  HRESULT ShowInternal(const NotificationOptions& options);
  HRESULT GetToastXml(
      ABI::Windows::UI::Notifications::IToastNotificationManagerStatics*
          toastManager,
      const std::u16string& title,
      const std::u16string& msg,
      const std::wstring& icon_path,
      const std::u16string& timeout_type,
      const bool silent,
      ABI::Windows::Data::Xml::Dom::IXmlDocument** toast_xml);
  HRESULT SetXmlAudioSilent(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc);
  HRESULT SetXmlScenarioReminder(
      ABI::Windows::Data::Xml::Dom::IXmlDocument* doc);
  HRESULT SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                     const std::u16string& text);
  HRESULT SetXmlText(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                     const std::u16string& title,
                     const std::u16string& body);
  HRESULT SetXmlImage(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                      const std::wstring& icon_path);
  HRESULT GetTextNodeList(
      ScopedHString* tag,
      ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
      ABI::Windows::Data::Xml::Dom::IXmlNodeList** node_list,
      uint32_t req_length);
  HRESULT AppendTextToXml(ABI::Windows::Data::Xml::Dom::IXmlDocument* doc,
                          ABI::Windows::Data::Xml::Dom::IXmlNode* node,
                          const std::u16string& text);
  HRESULT XmlDocumentFromString(
      const wchar_t* xmlString,
      ABI::Windows::Data::Xml::Dom::IXmlDocument** doc);
  HRESULT SetupCallbacks(
      ABI::Windows::UI::Notifications::IToastNotification* toast);
  bool RemoveCallbacks(
      ABI::Windows::UI::Notifications::IToastNotification* toast);

  static ComPtr<
      ABI::Windows::UI::Notifications::IToastNotificationManagerStatics>
      toast_manager_;
  static ComPtr<ABI::Windows::UI::Notifications::IToastNotifier>
      toast_notifier_;

  EventRegistrationToken activated_token_;
  EventRegistrationToken dismissed_token_;
  EventRegistrationToken failed_token_;

  ComPtr<ToastEventHandler> event_handler_;
  ComPtr<ABI::Windows::UI::Notifications::IToastNotification>
      toast_notification_;

  DISALLOW_COPY_AND_ASSIGN(WindowsToastNotification);
};

class ToastEventHandler : public RuntimeClass<RuntimeClassFlags<ClassicCom>,
                                              DesktopToastActivatedEventHandler,
                                              DesktopToastDismissedEventHandler,
                                              DesktopToastFailedEventHandler> {
 public:
  explicit ToastEventHandler(Notification* notification);
  ~ToastEventHandler() override;

  IFACEMETHODIMP Invoke(
      ABI::Windows::UI::Notifications::IToastNotification* sender,
      IInspectable* args) override;
  IFACEMETHODIMP Invoke(
      ABI::Windows::UI::Notifications::IToastNotification* sender,
      ABI::Windows::UI::Notifications::IToastDismissedEventArgs* e) override;
  IFACEMETHODIMP Invoke(
      ABI::Windows::UI::Notifications::IToastNotification* sender,
      ABI::Windows::UI::Notifications::IToastFailedEventArgs* e) override;

 private:
  base::WeakPtr<Notification> notification_;  // 弱小的裁判。

  DISALLOW_COPY_AND_ASSIGN(ToastEventHandler);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_NOTIFICATIONS_WIN_WINDOWS_TOAST_NOTIFICATION_H_
