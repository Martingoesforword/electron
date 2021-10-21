// 版权所有(C)2013 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_MENU_H_
#define SHELL_BROWSER_API_ELECTRON_API_MENU_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "gin/arguments.h"
#include "shell/browser/api/electron_api_base_window.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/pinnable.h"

namespace electron {

namespace api {

class Menu : public gin::Wrappable<Menu>,
             public gin_helper::EventEmitterMixin<Menu>,
             public gin_helper::Constructible<Menu>,
             public gin_helper::Pinnable<Menu>,
             public ElectronMenuModel::Delegate,
             public ElectronMenuModel::Observer {
 public:
  // Gin_helper：：可构造的。
  static gin::Handle<Menu> New(gin::Arguments* args);
  static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
      v8::Isolate*,
      v8::Local<v8::ObjectTemplate>);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;

#if defined(OS_MAC)
  // 设置全局菜单栏。
  static void SetApplicationMenu(Menu* menu);

  // 从应用程序菜单假发送操作。
  static void SendActionToFirstResponder(const std::string& action);
#endif

  ElectronMenuModel* model() const { return model_.get(); }

 protected:
  explicit Menu(gin::Arguments* args);
  ~Menu() override;

  // 返回一个新的回调，该回调保留JS包装的引用，直到。
  // 传入的|回调|被调用。
  base::OnceClosure BindSelfToClosure(base::OnceClosure callback);

  // UI：：SimpleMenuModel：：Delegate：
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;
  bool IsCommandIdVisible(int command_id) const override;
  bool ShouldCommandIdWorkWhenHidden(int command_id) const override;
  bool GetAcceleratorForCommandIdWithParams(
      int command_id,
      bool use_default_accelerator,
      ui::Accelerator* accelerator) const override;
  bool ShouldRegisterAcceleratorForCommandId(int command_id) const override;
#if defined(OS_MAC)
  bool GetSharingItemForCommandId(
      int command_id,
      ElectronMenuModel::SharingItem* item) const override;
  v8::Local<v8::Value> GetUserAcceleratorAt(int command_id) const;
#endif
  void ExecuteCommand(int command_id, int event_flags) override;
  void OnMenuWillShow(ui::SimpleMenuModel* source) override;

  virtual void PopupAt(BaseWindow* window,
                       int x,
                       int y,
                       int positioning_item,
                       base::OnceClosure callback) = 0;
  virtual void ClosePopupAt(int32_t window_id) = 0;
  virtual std::u16string GetAcceleratorTextAtForTesting(int index) const;

  std::unique_ptr<ElectronMenuModel> model_;
  Menu* parent_ = nullptr;

  // 可观察到的：
  void OnMenuWillClose() override;
  void OnMenuWillShow() override;

 private:
  void InsertItemAt(int index, int command_id, const std::u16string& label);
  void InsertSeparatorAt(int index);
  void InsertCheckItemAt(int index,
                         int command_id,
                         const std::u16string& label);
  void InsertRadioItemAt(int index,
                         int command_id,
                         const std::u16string& label,
                         int group_id);
  void InsertSubMenuAt(int index,
                       int command_id,
                       const std::u16string& label,
                       Menu* menu);
  void SetIcon(int index, const gfx::Image& image);
  void SetSublabel(int index, const std::u16string& sublabel);
  void SetToolTip(int index, const std::u16string& toolTip);
  void SetRole(int index, const std::u16string& role);
  void Clear();
  int GetIndexOfCommandId(int command_id) const;
  int GetItemCount() const;
  int GetCommandIdAt(int index) const;
  std::u16string GetLabelAt(int index) const;
  std::u16string GetSublabelAt(int index) const;
  std::u16string GetToolTipAt(int index) const;
  bool IsItemCheckedAt(int index) const;
  bool IsEnabledAt(int index) const;
  bool IsVisibleAt(int index) const;
  bool WorksWhenHiddenAt(int index) const;

  DISALLOW_COPY_AND_ASSIGN(Menu);
};

}  // 命名空间API。

}  // 命名空间电子。

namespace gin {

template <>
struct Converter<electron::ElectronMenuModel*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::ElectronMenuModel** out) {
    // NULL将被转移到NULL。
    if (val->IsNull()) {
      *out = nullptr;
      return true;
    }

    electron::api::Menu* menu;
    if (!Converter<electron::api::Menu*>::FromV8(isolate, val, &menu))
      return false;
    *out = menu->model();
    return true;
  }
};

}  // 命名空间杜松子酒。

#endif  // Shell_Browser_API_Electronics_API_Menu_H_
