// 版权所有(C)2020 Microsoft，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/drag_util.h"

#include <memory>

#include "ui/gfx/skia_util.h"

namespace electron {

// 返回填充一定大小的窗口的不可拖动区域的向量。
// |Width|by|Height|，但在窗口应该可以拖动的位置留有空隙。
std::vector<gfx::Rect> CalculateNonDraggableRegions(
    std::unique_ptr<SkRegion> draggable,
    int width,
    int height) {
  std::vector<gfx::Rect> result;
  SkRegion non_draggable;
  non_draggable.op({0, 0, width, height}, SkRegion::kUnion_Op);
  non_draggable.op(*draggable, SkRegion::kDifference_Op);
  for (SkRegion::Iterator it(non_draggable); !it.done(); it.next()) {
    result.push_back(gfx::SkIRectToRect(it.rect()));
  }
  return result;
}

// 将RAW格式的可拖动区域转换为SkRegion格式。
std::unique_ptr<SkRegion> DraggableRegionsToSkRegion(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  auto sk_region = std::make_unique<SkRegion>();
  for (const auto& region : regions) {
    sk_region->op(
        SkIRect::MakeLTRB(region->bounds.x(), region->bounds.y(),
                          region->bounds.right(), region->bounds.bottom()),
        region->draggable ? SkRegion::kUnion_Op : SkRegion::kDifference_Op);
  }
  return sk_region;
}

}  // 命名空间电子
