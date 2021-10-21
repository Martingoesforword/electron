// 版权所有2015年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_API_RESOURCES_PRIVATE_RESOURCES_PRIVATE_API_H_
#define SHELL_BROWSER_EXTENSIONS_API_RESOURCES_PRIVATE_RESOURCES_PRIVATE_API_H_

#include "base/macros.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

class ResourcesPrivateGetStringsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("resourcesPrivate.getStrings",
                             RESOURCESPRIVATE_GETSTRINGS)
  ResourcesPrivateGetStringsFunction();

 protected:
  ~ResourcesPrivateGetStringsFunction() override;

  // 从ExtensionFunction覆盖：
  ExtensionFunction::ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResourcesPrivateGetStringsFunction);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_API_RESOURCES_PRIVATE_RESOURCES_PRIVATE_API_H_
