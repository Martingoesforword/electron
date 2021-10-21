// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NATIVE_BROWSER_VIEW_H_
#define SHELL_BROWSER_NATIVE_BROWSER_VIEW_H_

#include <vector>

#include "base/macros.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "shell/common/api/api.mojom.h"
#include "third_party/skia/include/core/SkColor.h"

namespace gfx {
class Rect;
}

namespace electron {

enum AutoResizeFlags {
  kAutoResizeWidth = 0x1,
  kAutoResizeHeight = 0x2,
  kAutoResizeHorizontal = 0x4,
  kAutoResizeVertical = 0x8,
};

class InspectableWebContents;
class InspectableWebContentsView;

class NativeBrowserView : public content::WebContentsObserver {
 public:
  ~NativeBrowserView() override;

  static NativeBrowserView* Create(
      InspectableWebContents* inspectable_web_contents);

  InspectableWebContents* GetInspectableWebContents() {
    return inspectable_web_contents_;
  }

  const std::vector<mojom::DraggableRegionPtr>& GetDraggableRegions() const {
    return draggable_regions_;
  }

  InspectableWebContentsView* GetInspectableWebContentsView();

  virtual void SetAutoResizeFlags(uint8_t flags) = 0;
  virtual void SetBounds(const gfx::Rect& bounds) = 0;
  virtual gfx::Rect GetBounds() = 0;
  virtual void SetBackgroundColor(SkColor color) = 0;

  virtual void UpdateDraggableRegions(
      const std::vector<gfx::Rect>& drag_exclude_rects) {}

  // 当窗口需要更新其可拖动区域时调用。
  virtual void UpdateDraggableRegions(
      const std::vector<mojom::DraggableRegionPtr>& regions) {}

 protected:
  explicit NativeBrowserView(InspectableWebContents* inspectable_web_contents);
  // 内容：：WebContentsViewer：
  void WebContentsDestroyed() override;

  InspectableWebContents* inspectable_web_contents_;
  std::vector<mojom::DraggableRegionPtr> draggable_regions_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeBrowserView);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Native_Browser_VIEW_H_
