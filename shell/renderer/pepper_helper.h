// 版权所有(C)2012 Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_PEPPER_HELPER_H_
#define SHELL_RENDERER_PEPPER_HELPER_H_

#include "base/compiler_specific.h"
#include "base/component_export.h"
#include "base/macros.h"
#include "content/public/renderer/render_frame_observer.h"

// 此类侦听来自RenderFrame和。
// 附加插件支持所需的部件。
class PepperHelper : public content::RenderFrameObserver {
 public:
  explicit PepperHelper(content::RenderFrame* render_frame);
  ~PepperHelper() override;

  // 渲染帧观察者。
  void DidCreatePepperPlugin(content::RendererPpapiHost* host) override;
  void OnDestruct() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PepperHelper);
};

#endif  // Shell_渲染器_Pepper_Helper_H_
