// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_OSR_OSR_VIEW_PROXY_H_
#define SHELL_BROWSER_OSR_OSR_VIEW_PROXY_H_

#include <memory>

#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"

namespace electron {

class OffscreenViewProxy;

class OffscreenViewProxyObserver {
 public:
  virtual void OnProxyViewPaint(const gfx::Rect& damage_rect) = 0;
  virtual void ProxyViewDestroyed(OffscreenViewProxy* proxy) = 0;
};

class OffscreenViewProxy {
 public:
  explicit OffscreenViewProxy(views::View* view);
  ~OffscreenViewProxy();

  void SetObserver(OffscreenViewProxyObserver* observer);
  void RemoveObserver();

  const SkBitmap* GetBitmap() const;
  void SetBitmap(const SkBitmap& bitmap);

  const gfx::Rect& GetBounds();
  void SetBounds(const gfx::Rect& bounds);

  void OnEvent(ui::Event* event);

  void ResetView() { view_ = nullptr; }

 private:
  views::View* view_;

  gfx::Rect view_bounds_;
  std::unique_ptr<SkBitmap> view_bitmap_;

  OffscreenViewProxyObserver* observer_ = nullptr;
};

}  // 命名空间电子。

#endif  // Shell_Browser_OSR_OSR_VIEW_PROXY_H_
