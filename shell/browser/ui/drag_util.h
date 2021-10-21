// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_DRAG_UTIL_H_
#define SHELL_BROWSER_UI_DRAG_UTIL_H_

#include <memory>
#include <vector>

#include "shell/common/api/api.mojom.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image.h"

namespace base {
class FilePath;
}

namespace electron {

void DragFileItems(const std::vector<base::FilePath>& files,
                   const gfx::Image& icon,
                   gfx::NativeView view);

std::vector<gfx::Rect> CalculateNonDraggableRegions(
    std::unique_ptr<SkRegion> draggable,
    int width,
    int height);

// 将RAW格式的可拖动区域转换为SkRegion格式。
std::unique_ptr<SkRegion> DraggableRegionsToSkRegion(
    const std::vector<mojom::DraggableRegionPtr>& regions);

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Drag_Util_H_
