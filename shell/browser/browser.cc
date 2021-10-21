// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/browser.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/common/chrome_paths.h"
#include "shell/browser/browser_observer.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/login_handler.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/application_info.h"
#include "shell/common/electron_paths.h"
#include "shell/common/gin_helper/arguments.h"

namespace electron {

namespace {

// Call|Quit|Chromium完全启动后。
// 
// 这对于在“Ready”事件中立即退出非常重要，当。
// 某些初始化任务可能仍处于挂起状态，此时正在退出。
// 可能会在出口发生撞车事故。
void RunQuitClosure(base::OnceClosure quit) {
  // 在Linux/Windows上，“Ready”事件在“PreMainMessageLoopRun”中发出，
  // 确保我们在消息循环运行一次后退出。
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, std::move(quit));
}

}  // 命名空间。

#if defined(OS_WIN)
Browser::LaunchItem::LaunchItem() = default;
Browser::LaunchItem::~LaunchItem() = default;
Browser::LaunchItem::LaunchItem(const LaunchItem& other) = default;
#endif

Browser::LoginItemSettings::LoginItemSettings() = default;
Browser::LoginItemSettings::~LoginItemSettings() = default;
Browser::LoginItemSettings::LoginItemSettings(const LoginItemSettings& other) =
    default;

Browser::Browser() {
  WindowList::AddObserver(this);
}

Browser::~Browser() {
  WindowList::RemoveObserver(this);
}

// 静电。
Browser* Browser::Get() {
  return ElectronBrowserMainParts::Get()->browser();
}

#if defined(OS_WIN) || defined(OS_LINUX)
void Browser::Focus(gin::Arguments* args) {
  // 将焦点放在第一个可见窗口上。
  for (auto* const window : WindowList::GetWindows()) {
    if (window->IsVisible()) {
      window->Focus(true);
      break;
    }
  }
}
#endif

void Browser::Quit() {
  if (is_quitting_)
    return;

  is_quitting_ = HandleBeforeQuit();
  if (!is_quitting_)
    return;

  if (electron::WindowList::IsEmpty())
    NotifyAndShutdown();
  else
    electron::WindowList::CloseAllWindows();
}

void Browser::Exit(gin::Arguments* args) {
  int code = 0;
  args->GetNext(&code);

  if (!ElectronBrowserMainParts::Get()->SetExitCode(code)) {
    // 消息循环未准备好，请直接退出。
    exit(code);
  } else {
    // 准备在所有窗口关闭后退出。
    is_quitting_ = true;

    // 记住这个调用者，这样我们就不会发出无关的事件。
    is_exiting_ = true;

    // 在退出前一定要毁掉窗户，否则可能会发生不好的事情。
    if (electron::WindowList::IsEmpty()) {
      Shutdown();
    } else {
      // 与Quit()不同，我们不要求关闭窗口，而是销毁窗口。
      // 在没有询问的情况下。
      electron::WindowList::DestroyAllWindows();
    }
  }
}

void Browser::Shutdown() {
  if (is_shutdown_)
    return;

  is_shutdown_ = true;
  is_quitting_ = true;

  for (BrowserObserver& observer : observers_)
    observer.OnQuit();

  if (quit_main_message_loop_) {
    RunQuitClosure(std::move(quit_main_message_loop_));
  } else {
    // 没有可用的消息循环，因此我们处于早期阶段，请等待。
    // QUIT_MAIN_MESSAGE_LOOP_可用。
    // 现在退出将把废弃的进程抛在脑后。
  }
}

std::string Browser::GetVersion() const {
  std::string ret = OverriddenApplicationVersion();
  if (ret.empty())
    ret = GetExecutableFileVersion();
  return ret;
}

void Browser::SetVersion(const std::string& version) {
  OverriddenApplicationVersion() = version;
}

std::string Browser::GetName() const {
  std::string ret = OverriddenApplicationName();
  if (ret.empty())
    ret = GetExecutableFileProductName();
  return ret;
}

void Browser::SetName(const std::string& name) {
  OverriddenApplicationName() = name;
}

int Browser::GetBadgeCount() {
  return badge_count_;
}

bool Browser::OpenFile(const std::string& file_path) {
  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnOpenFile(&prevent_default, file_path);

  return prevent_default;
}

void Browser::OpenURL(const std::string& url) {
  for (BrowserObserver& observer : observers_)
    observer.OnOpenURL(url);
}

void Browser::Activate(bool has_visible_windows) {
  for (BrowserObserver& observer : observers_)
    observer.OnActivate(has_visible_windows);
}

void Browser::WillFinishLaunching() {
  for (BrowserObserver& observer : observers_)
    observer.OnWillFinishLaunching();
}

void Browser::DidFinishLaunching(base::DictionaryValue launch_info) {
  // 确保创建了userdata目录。
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::FilePath user_data;
  if (base::PathService::Get(chrome::DIR_USER_DATA, &user_data))
    base::CreateDirectoryAndGetError(user_data, nullptr);

  is_ready_ = true;
  if (ready_promise_) {
    ready_promise_->Resolve();
  }
  for (BrowserObserver& observer : observers_)
    observer.OnFinishLaunching(launch_info);
}

v8::Local<v8::Value> Browser::WhenReady(v8::Isolate* isolate) {
  if (!ready_promise_) {
    ready_promise_ = std::make_unique<gin_helper::Promise<void>>(isolate);
    if (is_ready()) {
      ready_promise_->Resolve();
    }
  }
  return ready_promise_->GetHandle();
}

void Browser::OnAccessibilitySupportChanged() {
  for (BrowserObserver& observer : observers_)
    observer.OnAccessibilitySupportChanged();
}

void Browser::PreMainMessageLoopRun() {
  for (BrowserObserver& observer : observers_) {
    observer.OnPreMainMessageLoopRun();
  }
}

void Browser::PreCreateThreads() {
  for (BrowserObserver& observer : observers_) {
    observer.OnPreCreateThreads();
  }
}

void Browser::SetMainMessageLoopQuitClosure(base::OnceClosure quit_closure) {
  if (is_shutdown_)
    RunQuitClosure(std::move(quit_closure));
  else
    quit_main_message_loop_ = std::move(quit_closure);
}

void Browser::NotifyAndShutdown() {
  if (is_shutdown_)
    return;

  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnWillQuit(&prevent_default);

  if (prevent_default) {
    is_quitting_ = false;
    return;
  }

  Shutdown();
}

bool Browser::HandleBeforeQuit() {
  bool prevent_default = false;
  for (BrowserObserver& observer : observers_)
    observer.OnBeforeQuit(&prevent_default);

  return !prevent_default;
}

void Browser::OnWindowCloseCancelled(NativeWindow* window) {
  if (is_quitting_)
    // 一旦预卸载处理程序阻止了关闭，我们认为退出。
    // 也被取消了。
    is_quitting_ = false;
}

void Browser::OnWindowAllClosed() {
  if (is_exiting_) {
    Shutdown();
  } else if (is_quitting_) {
    NotifyAndShutdown();
  } else {
    for (BrowserObserver& observer : observers_)
      observer.OnWindowAllClosed();
  }
}

#if defined(OS_MAC)
void Browser::NewWindowForTab() {
  for (BrowserObserver& observer : observers_)
    observer.OnNewWindowForTab();
}

void Browser::DidBecomeActive() {
  for (BrowserObserver& observer : observers_)
    observer.OnDidBecomeActive();
}
#endif

}  // 命名空间电子
