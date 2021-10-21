// 版权所有2019 Slake Technologies，Inc.保留所有权利。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。
#include "base/debug/stack_trace.h"

#include "shell/browser/extensions/electron_process_manager_delegate.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/one_shot_event.h"
#include "build/build_config.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/process_manager_factory.h"
#include "extensions/common/extension.h"
#include "extensions/common/permissions/permissions_data.h"

namespace extensions {

ElectronProcessManagerDelegate::ElectronProcessManagerDelegate() = default;
ElectronProcessManagerDelegate::~ElectronProcessManagerDelegate() = default;

bool ElectronProcessManagerDelegate::AreBackgroundPagesAllowedForContext(
    content::BrowserContext* context) const {
  return true;
}

bool ElectronProcessManagerDelegate::IsExtensionBackgroundPageAllowed(
    content::BrowserContext* context,
    const Extension& extension) const {
  return true;
}

bool ElectronProcessManagerDelegate::DeferCreatingStartupBackgroundHosts(
    content::BrowserContext* context) const {
  return false;
}

}  // 命名空间扩展
