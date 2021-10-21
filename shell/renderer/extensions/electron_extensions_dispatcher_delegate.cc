// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/extensions/electron_extensions_dispatcher_delegate.h"

#include <memory>
#include <set>
#include <string>

#include "chrome/renderer/extensions/extension_hooks_delegate.h"
#include "chrome/renderer/extensions/tabs_hooks_delegate.h"
#include "extensions/renderer/bindings/api_bindings_system.h"
#include "extensions/renderer/lazy_background_page_native_handler.h"
#include "extensions/renderer/module_system.h"
#include "extensions/renderer/native_extension_bindings_system.h"
#include "extensions/renderer/native_handler.h"

ElectronExtensionsDispatcherDelegate::ElectronExtensionsDispatcherDelegate() =
    default;

ElectronExtensionsDispatcherDelegate::~ElectronExtensionsDispatcherDelegate() =
    default;

void ElectronExtensionsDispatcherDelegate::RegisterNativeHandlers(
    extensions::Dispatcher* dispatcher,
    extensions::ModuleSystem* module_system,
    extensions::NativeExtensionBindingsSystem* bindings_system,
    extensions::ScriptContext* context) {
  module_system->RegisterNativeHandler(
      "lazy_background_page",
      std::make_unique<extensions::LazyBackgroundPageNativeHandler>(context));
}

void ElectronExtensionsDispatcherDelegate::PopulateSourceMap(
    extensions::ResourceBundleSourceMap* source_map) {}

void ElectronExtensionsDispatcherDelegate::RequireWebViewModules(
    extensions::ScriptContext* context) {}

void ElectronExtensionsDispatcherDelegate::OnActiveExtensionsUpdated(
    const std::set<std::string>& extension_ids) {}

void ElectronExtensionsDispatcherDelegate::InitializeBindingsSystem(
    extensions::Dispatcher* dispatcher,
    extensions::NativeExtensionBindingsSystem* bindings_system) {
  extensions::APIBindingsSystem* bindings = bindings_system->api_system();
  bindings->GetHooksForAPI("extension")
      ->SetDelegate(std::make_unique<extensions::ExtensionHooksDelegate>(
          bindings_system->messaging_service()));
  bindings->GetHooksForAPI("tabs")->SetDelegate(
      std::make_unique<extensions::TabsHooksDelegate>(
          bindings_system->messaging_service()));
}
