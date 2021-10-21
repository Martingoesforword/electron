// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_GLOBAL_SHORTCUT_H_
#define SHELL_BROWSER_API_ELECTRON_API_GLOBAL_SHORTCUT_H_

#include <map>
#include <vector>

#include "base/callback.h"
#include "chrome/browser/extensions/global_shortcut_listener.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "ui/base/accelerators/accelerator.h"

namespace electron {

namespace api {

class GlobalShortcut : public extensions::GlobalShortcutListener::Observer,
                       public gin::Wrappable<GlobalShortcut> {
 public:
  static gin::Handle<GlobalShortcut> Create(v8::Isolate* isolate);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  explicit GlobalShortcut(v8::Isolate* isolate);
  ~GlobalShortcut() override;

 private:
  typedef std::map<ui::Accelerator, base::RepeatingClosure>
      AcceleratorCallbackMap;

  bool RegisterAll(const std::vector<ui::Accelerator>& accelerators,
                   const base::RepeatingClosure& callback);
  bool Register(const ui::Accelerator& accelerator,
                const base::RepeatingClosure& callback);
  bool IsRegistered(const ui::Accelerator& accelerator);
  void Unregister(const ui::Accelerator& accelerator);
  void UnregisterSome(const std::vector<ui::Accelerator>& accelerators);
  void UnregisterAll();

  // GlobalShortcutListener：：观察者实现。
  void OnKeyPressed(const ui::Accelerator& accelerator) override;

  AcceleratorCallbackMap accelerator_callback_map_;

  DISALLOW_COPY_AND_ASSIGN(GlobalShortcut);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_GLOBAL_SHORTCUT_H_
