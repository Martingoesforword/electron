// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_SYSTEM_PREFERENCES_H_
#define SHELL_BROWSER_API_ELECTRON_API_SYSTEM_PREFERENCES_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"

#if defined(OS_WIN)
#include "shell/browser/browser.h"
#include "shell/browser/browser_observer.h"
#include "ui/gfx/sys_color_change_listener.h"
#endif

namespace electron {

namespace api {

#if defined(OS_MAC)
enum class NotificationCenterKind {
  kNSDistributedNotificationCenter = 0,
  kNSNotificationCenter,
  kNSWorkspaceNotificationCenter,
};
#endif

class SystemPreferences
    : public gin::Wrappable<SystemPreferences>,
      public gin_helper::EventEmitterMixin<SystemPreferences>
#if defined(OS_WIN)
    ,
      public BrowserObserver,
      public gfx::SysColorChangeListener
#endif
{
 public:
  static gin::Handle<SystemPreferences> Create(v8::Isolate* isolate);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

#if defined(OS_WIN) || defined(OS_MAC)
  std::string GetAccentColor();
  std::string GetColor(gin_helper::ErrorThrower thrower,
                       const std::string& color);
  std::string GetMediaAccessStatus(gin_helper::ErrorThrower thrower,
                                   const std::string& media_type);
#endif
#if defined(OS_WIN)
  bool IsAeroGlassEnabled();

  void InitializeWindow();

  // GFX：：SysColorChangeListener：
  void OnSysColorChange() override;

  // 浏览器观察者：
  void OnFinishLaunching(const base::DictionaryValue& launch_info) override;

#elif defined(OS_MAC)
  using NotificationCallback = base::RepeatingCallback<
      void(const std::string&, base::DictionaryValue, const std::string&)>;

  void PostNotification(const std::string& name,
                        base::DictionaryValue user_info,
                        gin::Arguments* args);
  int SubscribeNotification(const std::string& name,
                            const NotificationCallback& callback);
  void UnsubscribeNotification(int id);
  void PostLocalNotification(const std::string& name,
                             base::DictionaryValue user_info);
  int SubscribeLocalNotification(const std::string& name,
                                 const NotificationCallback& callback);
  void UnsubscribeLocalNotification(int request_id);
  void PostWorkspaceNotification(const std::string& name,
                                 base::DictionaryValue user_info);
  int SubscribeWorkspaceNotification(const std::string& name,
                                     const NotificationCallback& callback);
  void UnsubscribeWorkspaceNotification(int request_id);
  v8::Local<v8::Value> GetUserDefault(v8::Isolate* isolate,
                                      const std::string& name,
                                      const std::string& type);
  void RegisterDefaults(gin::Arguments* args);
  void SetUserDefault(const std::string& name,
                      const std::string& type,
                      gin::Arguments* args);
  void RemoveUserDefault(const std::string& name);
  bool IsSwipeTrackingFromScrollEventsEnabled();

  std::string GetSystemColor(gin_helper::ErrorThrower thrower,
                             const std::string& color);

  bool CanPromptTouchID();
  v8::Local<v8::Promise> PromptTouchID(v8::Isolate* isolate,
                                       const std::string& reason);

  static bool IsTrustedAccessibilityClient(bool prompt);

  v8::Local<v8::Promise> AskForMediaAccess(v8::Isolate* isolate,
                                           const std::string& media_type);

  // TODO(MarshallOfSound)：一旦我们。
  // 在一台莫哈韦机器上进行测试。
  v8::Local<v8::Value> GetEffectiveAppearance(v8::Isolate* isolate);
  v8::Local<v8::Value> GetAppLevelAppearance(v8::Isolate* isolate);
  void SetAppLevelAppearance(gin::Arguments* args);
#endif
  bool IsInvertedColorScheme();
  bool IsHighContrastColorScheme();
  v8::Local<v8::Value> GetAnimationSettings(v8::Isolate* isolate);

 protected:
  SystemPreferences();
  ~SystemPreferences() override;

#if defined(OS_MAC)
  int DoSubscribeNotification(const std::string& name,
                              const NotificationCallback& callback,
                              NotificationCenterKind kind);
  void DoUnsubscribeNotification(int request_id, NotificationCenterKind kind);
#endif

 private:
#if defined(OS_WIN)
  // 当消息进入我们的消息传递窗口时调用静态回调。
  static LRESULT CALLBACK WndProcStatic(HWND hwnd,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam);

  LRESULT CALLBACK WndProc(HWND hwnd,
                           UINT message,
                           WPARAM wparam,
                           LPARAM lparam);

  // |Window_|的窗口类。
  ATOM atom_;

  // 包含|Window_|的窗口过程的模块的句柄。
  HMODULE instance_;

  // 用于处理事件的窗口。
  HWND window_;

  std::string current_color_;

  bool invertered_color_scheme_ = false;

  bool high_contrast_color_scheme_ = false;

  std::unique_ptr<gfx::ScopedSysColorChangeListener> color_change_listener_;
#endif
  DISALLOW_COPY_AND_ASSIGN(SystemPreferences);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_SYSTEM_PREFERENCES_H_
