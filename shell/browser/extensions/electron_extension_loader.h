// 版权所有2018年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_LOADER_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_LOADER_H_

#include <string>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "extensions/browser/extension_registrar.h"
#include "extensions/common/extension_id.h"

namespace base {
class FilePath;
}

namespace content {
class BrowserContext;
}

namespace extensions {

class Extension;

// 使用ExtensionRegistry处理扩展加载和重新加载。
class ElectronExtensionLoader : public ExtensionRegistrar::Delegate {
 public:
  explicit ElectronExtensionLoader(content::BrowserContext* browser_context);
  ~ElectronExtensionLoader() override;

  // 从目录同步加载未打包的扩展名。返回。
  // 如果成功，则为Extension，否则为nullptr。
  void LoadExtension(const base::FilePath& extension_dir,
                     int load_flags,
                     base::OnceCallback<void(const Extension* extension,
                                             const std::string&)> cb);

  // 开始重新加载扩展。保持活动状态一直保持到。
  // 重新加载成功/失败。如果扩展模块是应用程序，则将在。
  // 重新装弹。
  // 这可能会使对旧扩展对象的引用无效，因此它将。
  // 按值显示的ID。
  void ReloadExtension(const ExtensionId& extension_id);

  void UnloadExtension(const ExtensionId& extension_id,
                       extensions::UnloadedExtensionReason reason);

  ExtensionRegistrar* registrar() { return &extension_registrar_; }

 private:
  // 如果扩展加载成功，则启用它。如果是应用程序，则会启动。
  // 它。如果加载失败，则更新ShellKeepAliveRequester。
  void FinishExtensionReload(
      const ExtensionId& old_extension_id,
      std::pair<scoped_refptr<const Extension>, std::string> result);

  void FinishExtensionLoad(
      base::OnceCallback<void(const Extension*, const std::string&)> cb,
      std::pair<scoped_refptr<const Extension>, std::string> result);

  // 扩展注册处：：代表：
  void PreAddExtension(const Extension* extension,
                       const Extension* old_extension) override;
  void PostActivateExtension(scoped_refptr<const Extension> extension) override;
  void PostDeactivateExtension(
      scoped_refptr<const Extension> extension) override;
  void LoadExtensionForReload(
      const ExtensionId& extension_id,
      const base::FilePath& path,
      ExtensionRegistrar::LoadErrorBehavior load_error_behavior) override;
  bool CanEnableExtension(const Extension* extension) override;
  bool CanDisableExtension(const Extension* extension) override;
  bool ShouldBlockExtension(const Extension* extension) override;

  content::BrowserContext* browser_context_;  // 不是所有的。

  // 注册和注销扩展。
  ExtensionRegistrar extension_registrar_;

  // 保存用于重新启动应用程序的Keep-Alive。
  // ShellKeepAliveRequester Keep_Alive_Requester_；

  // 指示我们发布了(异步)任务以开始重新加载。
  // 由ReloadExtension()用于检查ExtensionRegister是否调用。
  // LoadExtensionForReload()。
  bool did_schedule_reload_ = false;

  base::WeakPtrFactory<ElectronExtensionLoader> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionLoader);
};

}  // 命名空间扩展。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_LOADER_H_
