// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/cocoa/delayed_native_view_host.h"

namespace electron {

DelayedNativeViewHost::DelayedNativeViewHost(gfx::NativeView native_view)
    : native_view_(native_view) {}

DelayedNativeViewHost::~DelayedNativeViewHost() = default;

void DelayedNativeViewHost::ViewHierarchyChanged(
    const views::ViewHierarchyChangedDetails& details) {
  NativeViewHost::ViewHierarchyChanged(details);
  if (details.is_add && GetWidget())
    Attach(native_view_);
}

}  // 命名空间电子
