// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_

#include <stddef.h>

#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "extensions/browser/component_extension_resource_manager.h"
#include "ui/base/webui/resource_path.h"

struct GritResourceMap;

namespace extensions {

class ElectronComponentExtensionResourceManager
    : public ComponentExtensionResourceManager {
 public:
  ElectronComponentExtensionResourceManager();
  ~ElectronComponentExtensionResourceManager() override;

  // 从ComponentExtensionResourceManager覆盖：
  bool IsComponentExtensionResource(const base::FilePath& extension_path,
                                    const base::FilePath& resource_path,
                                    int* resource_id) const override;
  const ui::TemplateReplacements* GetTemplateReplacementsForExtension(
      const std::string& extension_id) const override;

 private:
  void AddComponentResourceEntries(const webui::ResourcePath* entries,
                                   size_t size);

  // 从资源路径到资源ID的映射。由。
  // IsComponentExtensionResource。
  std::map<base::FilePath, int> path_to_resource_id_;

  // 从扩展ID到其I18N模板替代项的映射。
  std::map<std::string, ui::TemplateReplacements>
      extension_template_replacements_;

  DISALLOW_COPY_AND_ASSIGN(ElectronComponentExtensionResourceManager);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
