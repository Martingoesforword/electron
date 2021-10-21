// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_MESSAGE_BOX_H_
#define SHELL_BROWSER_UI_MESSAGE_BOX_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/gfx/image/image_skia.h"

namespace electron {

class NativeWindow;

enum class MessageBoxType {
  kNone = 0,
  kInformation,
  kWarning,
  kError,
  kQuestion,
};

struct MessageBoxSettings {
  electron::NativeWindow* parent_window = nullptr;
  MessageBoxType type = electron::MessageBoxType::kNone;
  std::vector<std::string> buttons;
  absl::optional<int> id;
  int default_id;
  int cancel_id;
  bool no_link = false;
  std::string title;
  std::string message;
  std::string detail;
  std::string checkbox_label;
  bool checkbox_checked = false;
  gfx::ImageSkia icon;
  int text_width = 0;

  MessageBoxSettings();
  MessageBoxSettings(const MessageBoxSettings&);
  ~MessageBoxSettings();
};

int ShowMessageBoxSync(const MessageBoxSettings& settings);

typedef base::OnceCallback<void(int code, bool checkbox_checked)>
    MessageBoxCallback;

void ShowMessageBox(const MessageBoxSettings& settings,
                    MessageBoxCallback callback);

void CloseMessageBox(int id);

// 类似于设置最简单的ShowMessageBox，但在很早就可以安全地调用。
// 申请阶段。
void ShowErrorBox(const std::u16string& title, const std::u16string& content);

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Message_Box_H_
