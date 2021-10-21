// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_GUEST_VIEW_CONTAINER_H_
#define SHELL_RENDERER_GUEST_VIEW_CONTAINER_H_

#include "base/callback.h"

namespace content {
class RenderFrame;
}

namespace gfx {
class Size;
}

namespace electron {

class GuestViewContainer {
 public:
  typedef base::RepeatingCallback<void(const gfx::Size&)> ResizeCallback;

  explicit GuestViewContainer(content::RenderFrame* render_frame);
  virtual ~GuestViewContainer();

  static GuestViewContainer* FromID(int element_instance_id);

  void RegisterElementResizeCallback(const ResizeCallback& callback);
  void SetElementInstanceID(int element_instance_id);
  void DidResizeElement(const gfx::Size& new_size);

 private:
  int element_instance_id_;

  ResizeCallback element_resize_callback_;

  base::WeakPtrFactory<GuestViewContainer> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(GuestViewContainer);
};

}  // 命名空间电子。

#endif  // Shell_渲染器_Guest_VIEW_CONTAINER_H_
