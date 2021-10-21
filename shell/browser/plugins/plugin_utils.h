// 版权所有2016年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_PLUGINS_PLUGIN_UTILS_H_
#define SHELL_BROWSER_PLUGINS_PLUGIN_UTILS_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/macros.h"

namespace content {
class BrowserContext;
}

class PluginUtils {
 public:
  // 如果有允许处理|MIME_TYPE|的扩展，则返回其。
  // ID。否则返回空字符串。
  static std::string GetExtensionIdForMimeType(
      content::BrowserContext* browser_context,
      const std::string& mime_type);

  // 返回用MIME类型填充的映射，该MIME类型由扩展处理为。
  // 密钥和相应的扩展ID作为值。
  static base::flat_map<std::string, std::string> GetMimeTypeToExtensionIdMap(
      content::BrowserContext* browser_context);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PluginUtils);
};

#endif  // Shell_Browser_Plugin_Plugin_Utils_H_
