// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/message_box.h"

#include <windows.h>  // 必须先包含windows.h。

#include <commctrl.h>

#include <map>
#include <vector>

#include "base/containers/contains.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "base/task/post_task.h"
#include "base/win/scoped_gdi_object.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/win/dialog_thread.h"
#include "shell/browser/unresponsive_suppressor.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/image/image_skia.h"

namespace electron {

MessageBoxSettings::MessageBoxSettings() = default;
MessageBoxSettings::MessageBoxSettings(const MessageBoxSettings&) = default;
MessageBoxSettings::~MessageBoxSettings() = default;

namespace {

struct DialogResult {
  int button_id;
  bool verification_flag_checked;
};

// &lt;ID，messageBox&gt;映射。
// 
// 请注意，HWND存储在UNIQUE_PTR中，因为HWND的指针。
// 将在线程之间传递，我们需要确保HWND的内存。
// 修改对话框映射时未更改。
std::map<int, std::unique_ptr<HWND>>& GetDialogsMap() {
  static base::NoDestructor<std::map<int, std::unique_ptr<HWND>>> dialogs;
  return *dialogs;
}

// 对话框映射使用的特定HWND。
// 
// -ID已使用，但窗口尚未创建。
const HWND kHwndReserve = reinterpret_cast<HWND>(-1);
// -通知取消消息框。
const HWND kHwndCancel = reinterpret_cast<HWND>(-2);

// 用于修改线程间HWND的锁。
// 
// 请注意，可能同时打开了多个对话框，但是。
// 我们只对它们使用一个锁，因为每个对话框都独立于。
// 并且不需要对每一个都使用不同的锁。
// 另请注意，|GetDialogsMap|仅在主线程中使用，什么是。
// 线程之间共享的是HWND的内存，因此不需要使用锁。
// 当访问对话框地图时。
base::Lock& GetHWNDLock() {
  static base::NoDestructor<base::Lock> lock;
  return *lock;
}

// Windows已经采用了较小的命令ID值，我们必须从。
// 数量较大，以避免与Windows发生冲突。
const int kIDStart = 100;

// 从按钮的名称中获取公共ID。
struct CommonButtonID {
  int button;
  int id;
};
CommonButtonID GetCommonID(const std::wstring& button) {
  std::wstring lower = base::ToLowerASCII(button);
  if (lower == L"ok")
    return {TDCBF_OK_BUTTON, IDOK};
  else if (lower == L"yes")
    return {TDCBF_YES_BUTTON, IDYES};
  else if (lower == L"no")
    return {TDCBF_NO_BUTTON, IDNO};
  else if (lower == L"cancel")
    return {TDCBF_CANCEL_BUTTON, IDCANCEL};
  else if (lower == L"retry")
    return {TDCBF_RETRY_BUTTON, IDRETRY};
  else if (lower == L"close")
    return {TDCBF_CLOSE_BUTTON, IDCLOSE};
  return {-1, -1};
}

// 确定按钮是否为通用按钮，如果是，则映射通用ID。
// 设置为按钮ID。
void MapToCommonID(const std::vector<std::wstring>& buttons,
                   std::map<int, int>* id_map,
                   TASKDIALOG_COMMON_BUTTON_FLAGS* button_flags,
                   std::vector<TASKDIALOG_BUTTON>* dialog_buttons) {
  for (size_t i = 0; i < buttons.size(); ++i) {
    auto common = GetCommonID(buttons[i]);
    if (common.button != -1) {
      // 这是一个普通的按钮。
      (*id_map)[common.id] = i;
      (*button_flags) |= common.button;
    } else {
      // 这是一个自定义按钮。
      dialog_buttons->push_back(
          {static_cast<int>(i + kIDStart), buttons[i].c_str()});
    }
  }
}

// 任务对话框的回调。TaskDialogIndirect API不提供。
// HWND，我们必须监听TDN_CREATED消息才能获得。
// 它。
// 请注意，此回调在对话线程中运行，而不是在主线程中运行，因此它。
// CloseMessageBox可以在对话框之前调用或全部在对话框之后调用。
// 窗口已创建。
HRESULT CALLBACK
TaskDialogCallback(HWND hwnd, UINT msg, WPARAM, LPARAM, LONG_PTR data) {
  if (msg == TDN_CREATED) {
    HWND* target = reinterpret_cast<HWND*>(data);
    // 锁定，因为可能会调用CloseMessageBox。
    base::AutoLock lock(GetHWNDLock());
    if (*target == kHwndCancel) {
      // 该对话框在创建前已取消，请直接将其关闭。
      ::PostMessage(hwnd, WM_CLOSE, 0, 0);
    } else if (*target == kHwndReserve) {
      // 否则就救了HWND吧。
      *target = hwnd;
    } else {
      NOTREACHED();
    }
  }
  return S_OK;
}

DialogResult ShowTaskDialogWstr(NativeWindow* parent,
                                MessageBoxType type,
                                const std::vector<std::wstring>& buttons,
                                int default_id,
                                int cancel_id,
                                bool no_link,
                                const std::u16string& title,
                                const std::u16string& message,
                                const std::u16string& detail,
                                const std::u16string& checkbox_label,
                                bool checkbox_checked,
                                const gfx::ImageSkia& icon,
                                HWND* hwnd) {
  TASKDIALOG_FLAGS flags =
      TDF_SIZE_TO_CONTENT |           // 显示所有内容。
      TDF_ALLOW_DIALOG_CANCELLATION;  // 允许取消对话。

  TASKDIALOGCONFIG config = {0};
  config.cbSize = sizeof(config);
  config.hInstance = GetModuleHandle(NULL);
  config.dwFlags = flags;

  if (parent) {
    config.hwndParent = static_cast<electron::NativeWindowViews*>(parent)
                            ->GetAcceleratedWidget();
  }

  if (default_id > 0)
    config.nDefaultButton = kIDStart + default_id;

  // TaskDialogIndirect不允许名称为空，如果我们将标题设置为空。
  // 将在标题中显示“Electron.exe”。
  if (title.empty()) {
    std::wstring app_name = base::UTF8ToWide(Browser::Get()->GetName());
    config.pszWindowTitle = app_name.c_str();
  } else {
    config.pszWindowTitle = base::as_wcstr(title);
  }

  base::win::ScopedHICON hicon;
  if (!icon.isNull()) {
    hicon = IconUtil::CreateHICONFromSkBitmap(*icon.bitmap());
    config.dwFlags |= TDF_USE_HICON_MAIN;
    config.hMainIcon = hicon.get();
  } else {
    // 根据对话框类型显示图标。
    switch (type) {
      case MessageBoxType::kInformation:
      case MessageBoxType::kQuestion:
        config.pszMainIcon = TD_INFORMATION_ICON;
        break;
      case MessageBoxType::kWarning:
        config.pszMainIcon = TD_WARNING_ICON;
        break;
      case MessageBoxType::kError:
        config.pszMainIcon = TD_ERROR_ICON;
        break;
      case MessageBoxType::kNone:
        break;
    }
  }

  // 如果“详细信息”为空，则不要突出显示消息。
  if (detail.empty()) {
    config.pszContent = base::as_wcstr(message);
  } else {
    config.pszMainInstruction = base::as_wcstr(message);
    config.pszContent = base::as_wcstr(detail);
  }

  if (!checkbox_label.empty()) {
    config.pszVerificationText = base::as_wcstr(checkbox_label);
    if (checkbox_checked)
      config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;
  }

  // 遍历按钮，将常用按钮放入dwCommonButton。
  // 以及pButton中的自定义按钮。
  std::map<int, int> id_map;
  std::vector<TASKDIALOG_BUTTON> dialog_buttons;
  if (no_link) {
    for (size_t i = 0; i < buttons.size(); ++i)
      dialog_buttons.push_back(
          {static_cast<int>(i + kIDStart), buttons[i].c_str()});
  } else {
    MapToCommonID(buttons, &id_map, &config.dwCommonButtons, &dialog_buttons);
  }
  if (!dialog_buttons.empty()) {
    config.pButtons = &dialog_buttons.front();
    config.cButtons = dialog_buttons.size();
    if (!no_link)
      config.dwFlags |= TDF_USE_COMMAND_LINKS;  // 自定义按钮作为链接。
  }

  // 传递回调以接收消息框的HWND。
  if (hwnd) {
    config.pfCallback = &TaskDialogCallback;
    config.lpCallbackData = reinterpret_cast<LONG_PTR>(hwnd);
  }

  int id = 0;
  BOOL verification_flag_checked = FALSE;
  TaskDialogIndirect(&config, &id, nullptr, &verification_flag_checked);

  int button_id;
  if (id_map.find(id) != id_map.end())  // 公共按钮。
    button_id = id_map[id];
  else if (id >= kIDStart)  // 自定义按钮。
    button_id = id - kIDStart;
  else
    button_id = cancel_id;

  return {button_id, static_cast<bool>(verification_flag_checked)};
}

DialogResult ShowTaskDialogUTF8(const MessageBoxSettings& settings,
                                HWND* hwnd) {
  std::vector<std::wstring> buttons;
  for (const auto& button : settings.buttons)
    buttons.push_back(base::UTF8ToWide(button));

  const std::u16string title = base::UTF8ToUTF16(settings.title);
  const std::u16string message = base::UTF8ToUTF16(settings.message);
  const std::u16string detail = base::UTF8ToUTF16(settings.detail);
  const std::u16string checkbox_label =
      base::UTF8ToUTF16(settings.checkbox_label);

  return ShowTaskDialogWstr(
      settings.parent_window, settings.type, buttons, settings.default_id,
      settings.cancel_id, settings.no_link, title, message, detail,
      checkbox_label, settings.checkbox_checked, settings.icon, hwnd);
}

}  // 命名空间。

int ShowMessageBoxSync(const MessageBoxSettings& settings) {
  electron::UnresponsiveSuppressor suppressor;
  DialogResult result = ShowTaskDialogUTF8(settings, nullptr);
  return result.button_id;
}

void ShowMessageBox(const MessageBoxSettings& settings,
                    MessageBoxCallback callback) {
  // 该对话框是在一个新线程中创建的，因此我们还不知道它的HWND，请将。
  // 对话框图中的kHwndReserve。
  HWND* hwnd = nullptr;
  if (settings.id) {
    if (base::Contains(GetDialogsMap(), *settings.id))
      CloseMessageBox(*settings.id);
    auto it = GetDialogsMap().emplace(*settings.id,
                                      std::make_unique<HWND>(kHwndReserve));
    hwnd = it.first->second.get();
  }

  dialog_thread::Run(
      base::BindOnce(&ShowTaskDialogUTF8, settings, base::Unretained(hwnd)),
      base::BindOnce(
          [](MessageBoxCallback callback, absl::optional<int> id,
             DialogResult result) {
            if (id)
              GetDialogsMap().erase(*id);
            std::move(callback).Run(result.button_id,
                                    result.verification_flag_checked);
          },
          std::move(callback), settings.id));
}

void CloseMessageBox(int id) {
  auto it = GetDialogsMap().find(id);
  if (it == GetDialogsMap().end()) {
    LOG(ERROR) << "CloseMessageBox called with nonexistent ID";
    return;
  }
  HWND* hwnd = it->second.get();
  // 锁定，因为TaskDialogCallback可能正在保存对话框的HWND。
  base::AutoLock lock(GetHWNDLock());
  DCHECK(*hwnd != kHwndCancel);
  if (*hwnd == kHwndReserve) {
    // 如果对话框窗口尚未创建，则告诉它取消。
    *hwnd = kHwndCancel;
  } else {
    // 否则，发送消息将其关闭。
    ::PostMessage(*hwnd, WM_CLOSE, 0, 0);
  }
}

void ShowErrorBox(const std::u16string& title, const std::u16string& content) {
  electron::UnresponsiveSuppressor suppressor;
  ShowTaskDialogWstr(nullptr, MessageBoxType::kError, {}, -1, 0, false,
                     u"Error", title, content, u"", false, gfx::ImageSkia(),
                     nullptr);
}

}  // 命名空间电子
