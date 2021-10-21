// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_
#define SHELL_BROWSER_NATIVE_BROWSER_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>
#include <vector>

#include "base/mac/scoped_nsobject.h"
#include "shell/browser/native_browser_view.h"

namespace electron {

class NativeBrowserViewMac : public NativeBrowserView {
 public:
  explicit NativeBrowserViewMac(
      InspectableWebContents* inspectable_web_contents);
  ~NativeBrowserViewMac() override;

  void SetAutoResizeFlags(uint8_t flags) override;
  void SetBounds(const gfx::Rect& bounds) override;
  gfx::Rect GetBounds() override;
  void SetBackgroundColor(SkColor color) override;

  void UpdateDraggableRegions(
      const std::vector<mojom::DraggableRegionPtr>& regions) override;

  void UpdateDraggableRegions(
      const std::vector<gfx::Rect>& drag_exclude_rects) override;

  DISALLOW_COPY_AND_ASSIGN(NativeBrowserViewMac);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Native_Browser_VIEW_MAC_H_
