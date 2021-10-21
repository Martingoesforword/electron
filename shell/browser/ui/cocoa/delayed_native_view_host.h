// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_DELAYED_NATIVE_VIEW_HOST_H_
#define SHELL_BROWSER_UI_COCOA_DELAYED_NATIVE_VIEW_HOST_H_

#include "ui/views/controls/native/native_view_host.h"

namespace electron {

// 将NativeViewHost连接到后自动附加本地视图。
// 一个小工具。(直接连接会导致崩溃。)。
class DelayedNativeViewHost : public views::NativeViewHost {
 public:
  explicit DelayedNativeViewHost(gfx::NativeView native_view);
  ~DelayedNativeViewHost() override;

  // 视图：：视图：
  void ViewHierarchyChanged(
      const views::ViewHierarchyChangedDetails& details) override;

 private:
  gfx::NativeView native_view_;

  DISALLOW_COPY_AND_ASSIGN(DelayedNativeViewHost);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_COCOA_DELAYED_NATIVE_VIEW_HOST_H_
