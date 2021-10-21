// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_ELECTRON_MENU_MODEL_H_
#define SHELL_BROWSER_UI_ELECTRON_MENU_MODEL_H_

#include <map>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/models/simple_menu_model.h"
#include "url/gurl.h"

namespace electron {

class ElectronMenuModel : public ui::SimpleMenuModel {
 public:
#if defined(OS_MAC)
  struct SharingItem {
    SharingItem();
    SharingItem(SharingItem&&);
    SharingItem(const SharingItem&) = delete;
    ~SharingItem();

    absl::optional<std::vector<std::string>> texts;
    absl::optional<std::vector<GURL>> urls;
    absl::optional<std::vector<base::FilePath>> file_paths;
  };
#endif

  class Delegate : public ui::SimpleMenuModel::Delegate {
   public:
    ~Delegate() override {}

    virtual bool GetAcceleratorForCommandIdWithParams(
        int command_id,
        bool use_default_accelerator,
        ui::Accelerator* accelerator) const = 0;

    virtual bool ShouldRegisterAcceleratorForCommandId(
        int command_id) const = 0;

    virtual bool ShouldCommandIdWorkWhenHidden(int command_id) const = 0;

#if defined(OS_MAC)
    virtual bool GetSharingItemForCommandId(int command_id,
                                            SharingItem* item) const = 0;
#endif

   private:
    // UI：：SimpleMenuModel：：Delegate：
    bool GetAcceleratorForCommandId(
        int command_id,
        ui::Accelerator* accelerator) const override;
  };

  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    // 通知菜单将打开。
    virtual void OnMenuWillShow() {}

    // 通知菜单已关闭。
    virtual void OnMenuWillClose() {}
  };

  explicit ElectronMenuModel(Delegate* delegate);
  ~ElectronMenuModel() override;

  void AddObserver(Observer* obs) { observers_.AddObserver(obs); }
  void RemoveObserver(Observer* obs) { observers_.RemoveObserver(obs); }

  void SetToolTip(int index, const std::u16string& toolTip);
  std::u16string GetToolTipAt(int index);
  void SetRole(int index, const std::u16string& role);
  std::u16string GetRoleAt(int index);
  void SetSecondaryLabel(int index, const std::u16string& sublabel);
  std::u16string GetSecondaryLabelAt(int index) const override;
  bool GetAcceleratorAtWithParams(int index,
                                  bool use_default_accelerator,
                                  ui::Accelerator* accelerator) const;
  bool ShouldRegisterAcceleratorAt(int index) const;
  bool WorksWhenHiddenAt(int index) const;
#if defined(OS_MAC)
  // 返回菜单项的SharingItem。
  bool GetSharingItemAt(int index, SharingItem* item) const;
  // 设置/获取此菜单的SharingItem。
  void SetSharingItem(SharingItem item);
  const absl::optional<SharingItem>& GetSharingItem() const;
#endif

  // UI：：SimpleMenuModel：
  void MenuWillClose() override;
  void MenuWillShow() override;

  base::WeakPtr<ElectronMenuModel> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  using SimpleMenuModel::GetSubmenuModelAt;
  ElectronMenuModel* GetSubmenuModelAt(int index);

 private:
  Delegate* delegate_;  // 弱小的裁判。

#if defined(OS_MAC)
  absl::optional<SharingItem> sharing_item_;
#endif

  std::map<int, std::u16string> toolTips_;   // 命令id-&gt;工具提示。
  std::map<int, std::u16string> roles_;      // 命令id-&gt;角色。
  std::map<int, std::u16string> sublabels_;  // 命令ID-&gt;子标签。
  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<ElectronMenuModel> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ElectronMenuModel);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_Electronics_Menu_Model_H_
