// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_DISPATCHER_DELEGATE_H_
#define SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_DISPATCHER_DELEGATE_H_

#include <set>
#include <string>

#include "base/macros.h"
#include "extensions/renderer/dispatcher_delegate.h"

class ElectronExtensionsDispatcherDelegate
    : public extensions::DispatcherDelegate {
 public:
  ElectronExtensionsDispatcherDelegate();
  ~ElectronExtensionsDispatcherDelegate() override;

 private:
  // 扩展：：DispatcherDelegate实现。
  void RegisterNativeHandlers(
      extensions::Dispatcher* dispatcher,
      extensions::ModuleSystem* module_system,
      extensions::NativeExtensionBindingsSystem* bindings_system,
      extensions::ScriptContext* context) override;
  void PopulateSourceMap(
      extensions::ResourceBundleSourceMap* source_map) override;
  void RequireWebViewModules(extensions::ScriptContext* context) override;
  void OnActiveExtensionsUpdated(
      const std::set<std::string>& extension_ids) override;
  void InitializeBindingsSystem(
      extensions::Dispatcher* dispatcher,
      extensions::NativeExtensionBindingsSystem* bindings_system) override;

  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionsDispatcherDelegate);
};

#endif  // SHELL_RENDERER_EXTENSIONS_ELECTRON_EXTENSIONS_DISPATCHER_DELEGATE_H_
