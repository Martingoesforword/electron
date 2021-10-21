// 版权所有(C)2020 Microsoft，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENDED_WEB_CONTENTS_OBSERVER_H_
#define SHELL_BROWSER_EXTENDED_WEB_CONTENTS_OBSERVER_H_

#include <string>
#include <vector>

#include "base/observer_list.h"
#include "electron/shell/common/api/api.mojom.h"
#include "ui/gfx/geometry/rect.h"

namespace electron {

// 某些事件仅在WebContentsDelegate中，因此我们提供自己的。
// 派观察员调度这些事件。
class ExtendedWebContentsObserver : public base::CheckedObserver {
 public:
  virtual void OnCloseContents() {}
  virtual void OnDraggableRegionsUpdated(
      const std::vector<mojom::DraggableRegionPtr>& regions) {}
  virtual void OnSetContentBounds(const gfx::Rect& rect) {}
  virtual void OnActivateContents() {}
  virtual void OnPageTitleUpdated(const std::u16string& title,
                                  bool explicit_set) {}
  virtual void OnDevToolsResized() {}

 protected:
  ~ExtendedWebContentsObserver() override {}
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_EXTENDED_WEB_CONTENTS_OBSERVER_H_
