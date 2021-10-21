// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_BROWSER_H_
#define SHELL_BROWSER_BROWSER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/values.h"
#include "gin/dictionary.h"
#include "shell/browser/browser_observer.h"
#include "shell/browser/window_list_observer.h"
#include "shell/common/gin_helper/promise.h"

#if defined(OS_WIN)
#include <windows.h>
#include "base/files/file_path.h"
#include "shell/browser/ui/win/taskbar_host.h"
#endif

#if defined(OS_MAC)
#include "ui/base/cocoa/secure_password_input.h"
#endif

namespace base {
class FilePath;
}

namespace gin_helper {
class Arguments;
}

namespace electron {

class ElectronMenuModel;

// 此类用于控制应用程序范围的操作。
class Browser : public WindowListObserver {
 public:
  Browser();
  ~Browser() override;

  static Browser* Get();

  // 尝试关闭所有窗口并退出应用程序。
  void Quit();

  // 立即退出应用程序并设置退出代码。
  void Exit(gin::Arguments* args);

  // 清理所有内容并正常关闭应用程序。
  void Shutdown();

  // 聚焦应用程序。
  void Focus(gin::Arguments* args);

  // 返回可执行文件(或包)的版本。
  std::string GetVersion() const;

  // 覆盖应用程序版本。
  void SetVersion(const std::string& version);

  // 返回应用程序的名称，默认值为Just Electron。
  std::string GetName() const;

  // 覆盖应用程序名称。
  void SetName(const std::string& name);

  // 将|路径|添加到最近使用的文档列表。
  void AddRecentDocument(const base::FilePath& path);

  // 清除最近的文档列表。
  void ClearRecentDocuments();

#if defined(OS_WIN)
  // 设置应用程序用户模型ID。
  void SetAppUserModelID(const std::wstring& name);
#endif

  // 删除默认协议处理程序注册表项。
  bool RemoveAsDefaultProtocolClient(const std::string& protocol,
                                     gin::Arguments* args);

  // 设置为协议的默认处理程序。
  bool SetAsDefaultProtocolClient(const std::string& protocol,
                                  gin::Arguments* args);

  // 查询协议的默认处理程序的当前状态。
  bool IsDefaultProtocolClient(const std::string& protocol,
                               gin::Arguments* args);

  std::u16string GetApplicationNameForProtocol(const GURL& url);

#if !defined(OS_LINUX)
  // 获取应用程序的名称、图标和路径。
  v8::Local<v8::Promise> GetApplicationInfoForProtocol(v8::Isolate* isolate,
                                                       const GURL& url);
#endif

  // 设置/获取徽章计数。
  bool SetBadgeCount(absl::optional<int> count);
  int GetBadgeCount();

#if defined(OS_WIN)
  struct LaunchItem {
    std::wstring name;
    std::wstring path;
    std::wstring scope;
    std::vector<std::wstring> args;
    bool enabled = true;

    LaunchItem();
    ~LaunchItem();
    LaunchItem(const LaunchItem&);
  };
#endif

  // 设置/获取应用的登录项目设置。
  struct LoginItemSettings {
    bool open_at_login = false;
    bool open_as_hidden = false;
    bool restore_state = false;
    bool opened_at_login = false;
    bool opened_as_hidden = false;
    std::u16string path;
    std::vector<std::u16string> args;

#if defined(OS_WIN)
    // 在Browser：：setLoginItemSettings中使用。
    bool enabled = true;
    std::wstring name;

    // 在Browser：：getLoginItemSettings中使用。
    bool executable_will_launch_at_login = false;
    std::vector<LaunchItem> launch_items;
#endif

    LoginItemSettings();
    ~LoginItemSettings();
    LoginItemSettings(const LoginItemSettings&);
  };
  void SetLoginItemSettings(LoginItemSettings settings);
  LoginItemSettings GetLoginItemSettings(const LoginItemSettings& options);

#if defined(OS_MAC)
  // 设置决定是否关闭的处理程序。
  void SetShutdownHandler(base::RepeatingCallback<bool()> handler);

  // 隐藏应用程序。
  void Hide();

  // 显示应用程序。
  void Show();

  // 创建活动并将其设置为当前正在使用的活动。
  void SetUserActivity(const std::string& type,
                       base::DictionaryValue user_info,
                       gin::Arguments* args);

  // 返回当前用户活动的类型名称。
  std::string GetCurrentActivityType();

  // 使活动无效，并将其标记为不再符合。
  // 续写。
  void InvalidateCurrentActivity();

  // 将此活动对象标记为非活动，而不使其无效。
  void ResignCurrentActivity();

  // 更新当前用户活动。
  void UpdateCurrentActivity(const std::string& type,
                             base::DictionaryValue user_info);

  // 指示用户活动即将恢复。
  bool WillContinueUserActivity(const std::string& type);

  // 表示无法恢复转接活动。
  void DidFailToContinueUserActivity(const std::string& type,
                                     const std::string& error);

  // 通过切换恢复活动。
  bool ContinueUserActivity(const std::string& type,
                            base::DictionaryValue user_info,
                            base::DictionaryValue details);

  // 表示活动在另一台设备上继续。
  void UserActivityWasContinued(const std::string& type,
                                base::DictionaryValue user_info);

  // 提供更新转接有效负载的机会。
  bool UpdateUserActivityState(const std::string& type,
                               base::DictionaryValue user_info);

  // 弹出坞站图标。
  enum class BounceType{
      kCritical = 0,        // NSCriticalRequest。
      kInformational = 10,  // NSInformationalRequest。
  };
  int DockBounce(BounceType type);
  void DockCancelBounce(int request_id);

  // 弹出下载堆栈。
  void DockDownloadFinished(const std::string& filePath);

  // 设置/获取码头的徽章文本。
  void DockSetBadgeText(const std::string& label);
  std::string DockGetBadgeText();

  // 隐藏/显示停靠。
  void DockHide();
  v8::Local<v8::Promise> DockShow(v8::Isolate* isolate);
  bool DockIsVisible();

  // 设置码头菜单。
  void DockSetMenu(ElectronMenuModel* model);

  // 设置码头的图标。
  void DockSetIcon(v8::Isolate* isolate, v8::Local<v8::Value> icon);

#endif  // 已定义(OS_MAC)。

  void ShowAboutPanel();
  void SetAboutPanelOptions(base::DictionaryValue options);

#if defined(OS_MAC) || defined(OS_WIN)
  void ShowEmojiPanel();
#endif

#if defined(OS_WIN)
  struct UserTask {
    base::FilePath program;
    std::wstring arguments;
    std::wstring title;
    std::wstring description;
    base::FilePath working_dir;
    base::FilePath icon_path;
    int icon_index;

    UserTask();
    UserTask(const UserTask&);
    ~UserTask();
  };

  // 将自定义任务添加到跳转列表。
  bool SetUserTasks(const std::vector<UserTask>& tasks);

  // 返回应用程序用户模型ID，如果没有，则创建。
  // 其中一个来自APP的名字。
  // 返回的字符串由浏览器管理，不能修改。
  PCWSTR GetAppUserModelID();
#endif  // 已定义(OS_WIN)。

#if defined(OS_LINUX)
  // Unity Launcher是否正在运行。
  bool IsUnityRunning();
#endif  // 已定义(OS_Linux)。

  // 告诉应用程序打开一个文件。
  bool OpenFile(const std::string& file_path);

  // 告诉应用程序打开一个URL。
  void OpenURL(const std::string& url);

#if defined(OS_MAC)
  // 告诉应用程序为选项卡创建一个新窗口。
  void NewWindowForTab();

  // 告诉应用程序该应用程序已变为活动状态。
  void DidBecomeActive();
#endif  // 已定义(OS_MAC)。

  // 告诉应用程序已使用可见/不可见激活应用程序。
  // 窗户。
  void Activate(bool has_visible_windows);

  bool IsEmojiPanelSupported();

  // 告诉应用程序加载已经完成。
  void WillFinishLaunching();
  void DidFinishLaunching(base::DictionaryValue launch_info);

  void OnAccessibilitySupportChanged();

  void PreMainMessageLoopRun();
  void PreCreateThreads();

  // 存储提供的|QUIT_CLOSURE|，在最后一个浏览器。
  // 实例已销毁。
  void SetMainMessageLoopQuitClosure(base::OnceClosure quit_closure);

  void AddObserver(BrowserObserver* obs) { observers_.AddObserver(obs); }

  void RemoveObserver(BrowserObserver* obs) { observers_.RemoveObserver(obs); }

#if defined(OS_MAC)
  // 返回是否启用安全输入
  bool IsSecureKeyboardEntryEnabled();
  void SetSecureKeyboardEntryEnabled(bool enabled);
#endif

  bool is_shutting_down() const { return is_shutdown_; }
  bool is_quitting() const { return is_quitting_; }
  bool is_ready() const { return is_ready_; }
  v8::Local<v8::Value> WhenReady(v8::Isolate* isolate);

 protected:
  // 返回应用程序捆绑包或可执行文件的版本。
  std::string GetExecutableFileVersion() const;

  // 返回应用程序包或可执行文件的名称。
  std::string GetExecutableFileProductName() const;

  // 发送Will-Quit消息，然后关闭应用程序。
  void NotifyAndShutdown();

  // 发送退出前消息并开始关闭窗口。
  bool HandleBeforeQuit();

  bool is_quitting_ = false;

 private:
  // WindowListWatch实现：
  void OnWindowCloseCancelled(NativeWindow* window) override;
  void OnWindowAllClosed() override;

  // 浏览器的观察者。
  base::ObserverList<BrowserObserver> observers_;

  // 跟踪请求文件图标的任务。
  base::CancelableTaskTracker cancelable_task_tracker_;

  // 是否已调用`app.exit()`。
  bool is_exiting_ = false;

  // 是否已发出“Ready”事件。
  bool is_ready_ = false;

  // 浏览器正在关闭。
  bool is_shutdown_ = false;

  // 空，直到/除非默认的主消息循环正在运行。
  base::OnceClosure quit_main_message_loop_;

  int badge_count_ = 0;

  std::unique_ptr<gin_helper::Promise<void>> ready_promise_;

#if defined(OS_MAC)
  std::unique_ptr<ui::ScopedPasswordInputEnabler> password_input_enabler_;
  base::Time last_dock_show_;
#endif

#if defined(OS_LINUX) || defined(OS_WIN)
  base::Value about_panel_options_;
#elif defined(OS_MAC)
  base::DictionaryValue about_panel_options_;
#endif

#if defined(OS_WIN)
  void UpdateBadgeContents(HWND hwnd,
                           const absl::optional<std::string>& badge_content,
                           const std::string& badge_alt_string);

  // 负责任务栏相关API的运行。
  TaskbarHost taskbar_host_;
#endif

  DISALLOW_COPY_AND_ASSIGN(Browser);
};

}  // 命名空间电子。

#endif  // 外壳浏览器浏览器H_
