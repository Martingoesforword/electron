// 版权所有2016年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/plugins/plugin_utils.h"

#include <vector>

#include "base/containers/contains.h"
#include "base/values.h"
#include "content/public/common/webplugininfo.h"
#include "extensions/buildflags/buildflags.h"
#include "url/gurl.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/mime_types_handler.h"
#endif

// 静电。
std::string PluginUtils::GetExtensionIdForMimeType(
    content::BrowserContext* browser_context,
    const std::string& mime_type) {
  auto map = GetMimeTypeToExtensionIdMap(browser_context);
  auto it = map.find(mime_type);
  if (it != map.end())
    return it->second;
  return std::string();
}

base::flat_map<std::string, std::string>
PluginUtils::GetMimeTypeToExtensionIdMap(
    content::BrowserContext* browser_context) {
  base::flat_map<std::string, std::string> mime_type_to_extension_id_map;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::vector<std::string> allowed_extension_ids =
      MimeTypesHandler::GetMIMETypeAllowlist();
  // 检查白名单中的分机并尝试使用它们拦截。
  // URL请求。
  for (const std::string& extension_id : allowed_extension_ids) {
    const extensions::Extension* extension =
        extensions::ExtensionRegistry::Get(browser_context)
            ->enabled_extensions()
            .GetByID(extension_id);
    // 白名单上的扩展可能没有安装，因此我们必须将其设为空。
    // 选中|分机|。
    if (!extension) {
      continue;
    }

    if (MimeTypesHandler* handler = MimeTypesHandler::GetHandler(extension)) {
      for (const auto& supported_mime_type : handler->mime_type_set()) {
        DCHECK(!base::Contains(mime_type_to_extension_id_map,
                               supported_mime_type));
        mime_type_to_extension_id_map[supported_mime_type] = extension_id;
      }
    }
  }
#endif
  return mime_type_to_extension_id_map;
}
