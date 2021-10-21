// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_DESKTOP_CAPTURER_H_
#define SHELL_BROWSER_API_ELECTRON_API_DESKTOP_CAPTURER_H_

#include <memory>
#include <string>
#include <vector>

#include "chrome/browser/media/webrtc/desktop_media_list_observer.h"
#include "chrome/browser/media/webrtc/native_desktop_media_list.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/pinnable.h"

namespace electron {

namespace api {

class DesktopCapturer : public gin::Wrappable<DesktopCapturer>,
                        public gin_helper::Pinnable<DesktopCapturer>,
                        public DesktopMediaListObserver {
 public:
  struct Source {
    DesktopMediaList::Source media_list_source;
    // 如果不可用，将为空字符串。
    std::string display_id;

    // 此源是否应提供图标。
    bool fetch_icon = false;
  };

  static gin::Handle<DesktopCapturer> Create(v8::Isolate* isolate);

  void StartHandling(bool capture_window,
                     bool capture_screen,
                     const gfx::Size& thumbnail_size,
                     bool fetch_window_icons);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  explicit DesktopCapturer(v8::Isolate* isolate);
  ~DesktopCapturer() override;

  // DesktopMediaListViewer：
  void OnSourceAdded(DesktopMediaList* list, int index) override {}
  void OnSourceRemoved(DesktopMediaList* list, int index) override {}
  void OnSourceMoved(DesktopMediaList* list,
                     int old_index,
                     int new_index) override {}
  void OnSourceNameChanged(DesktopMediaList* list, int index) override {}
  void OnSourceThumbnailChanged(DesktopMediaList* list, int index) override {}

 private:
  void UpdateSourcesList(DesktopMediaList* list);

  std::unique_ptr<DesktopMediaList> window_capturer_;
  std::unique_ptr<DesktopMediaList> screen_capturer_;
  std::vector<DesktopCapturer::Source> captured_sources_;
  bool capture_window_ = false;
  bool capture_screen_ = false;
  bool fetch_window_icons_ = false;
#if defined(OS_WIN)
  bool using_directx_capturer_ = false;
#endif  // 已定义(OS_WIN)。

  base::WeakPtrFactory<DesktopCapturer> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(DesktopCapturer);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_DESKTOP_CAPTURER_H_
