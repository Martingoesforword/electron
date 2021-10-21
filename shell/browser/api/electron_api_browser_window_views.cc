// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_browser_window.h"

#include "content/browser/web_contents/web_contents_impl.h"  // 点名检查。
#include "shell/browser/native_window_views.h"
#include "ui/aura/window.h"

namespace electron {

namespace api {

void BrowserWindow::UpdateDraggableRegions(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  if (window_->has_frame())
    return;

  if (&draggable_regions_ != &regions && web_contents()) {
    auto* view =
        static_cast<content::WebContentsImpl*>(web_contents())->GetView();
    if (view) {
      const gfx::NativeView nv = view->GetNativeView();
      auto const offset = nv->GetBoundsInRootWindow();
      auto snapped_regions = mojo::Clone(regions);
      for (auto& snapped_region : snapped_regions) {
        snapped_region->bounds.Offset(offset.x(), offset.y());
      }

      draggable_regions_ = mojo::Clone(snapped_regions);
    }
  }

  static_cast<NativeWindowViews*>(window_.get())
      ->UpdateDraggableRegions(draggable_regions_);
}

}  // 命名空间API。

}  // 命名空间电子
