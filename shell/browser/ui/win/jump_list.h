// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_WIN_JUMP_LIST_H_
#define SHELL_BROWSER_UI_WIN_JUMP_LIST_H_

#include <atlbase.h>
#include <shobjidl.h>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"

namespace electron {

enum class JumpListResult : int {
  kSuccess = 0,
  // 在JS代码中，此错误将显示为异常。
  kArgumentError = 1,
  // 一般错误，运行时日志可能会提供一些线索。
  kGenericError = 2,
  // 自定义类别不能包含分隔符。
  kCustomCategorySeparatorError = 3,
  // 该应用程序未注册为处理在自定义类别中找到的文件类型。
  kMissingFileTypeRegistrationError = 4,
  // 由于用户隐私设置，无法创建自定义类别。
  kCustomCategoryAccessDeniedError = 5,
};

struct JumpListItem {
  enum class Type {
    // 任务将启动一个应用程序(通常是创建跳转列表的应用程序)。
    // 带着特定的论据。
    kTask,
    // 分隔符只能插入标准任务中的项目之间。
    // 类别，则它们不能出现在自定义类别中。
    kSeparator,
    // 文件链接将使用创建跳转列表的应用程序打开文件，
    // 要进行此操作，必须将应用程序注册为该文件的处理程序。
    // 输入(尽管应用程序不必是默认处理程序)。
    kFile
  };

  Type type = Type::kTask;
  // 对于任务，这是程序可执行文件的路径；对于文件链接，这是。
  // 是完整的文件名。
  base::FilePath path;
  std::wstring arguments;
  std::wstring title;
  std::wstring description;
  base::FilePath working_dir;
  base::FilePath icon_path;
  int icon_index = 0;

  JumpListItem();
  JumpListItem(const JumpListItem&);
  ~JumpListItem();
};

struct JumpListCategory {
  enum class Type {
    // 自定义类别可以包含任务和文件，但不能包含分隔符。
    kCustom,
    // 频繁/最近的类别由操作系统、其名称和项目进行管理。
    // 不能由应用程序设置(但可以间接设置项目)。
    kFrequent,
    kRecent,
    // 应用程序无法重命名标准任务类别，但应用程序。
    // 可以设置应该出现在此类别中的项目，以及那些项目。
    // 可以包括任务、文件和分隔符。
    kTasks
  };

  Type type = Type::kTasks;
  std::wstring name;
  std::vector<JumpListItem> items;

  JumpListCategory();
  JumpListCategory(const JumpListCategory&);
  ~JumpListCategory();
};

// 创建或删除应用程序的自定义跳转列表。
// 请参阅https://msdn.microsoft.com/en-us/library/windows/desktop/gg281362.aspx。
class JumpList {
 public:
  // |app_id|必须是应用的应用用户型号ID。
  // 自定义跳转列表应创建/删除，通常通过以下方式获取。
  // 正在调用GetCurrentProcessExplitAppUserModelID()。
  explicit JumpList(const std::wstring& app_id);
  ~JumpList();

  // 启动新事务，必须在追加任何类别之前调用，
  // 放弃或提交。方法返回后|MIN_ITEMS|将指示。
  // 将在跳转列表中显示的最小项目数，以及。
  // |REMOVERED_ITEMS|(如果不为空)将包含用户拥有的所有项目。
  // 从跳转列表中取消固定。这两个参数都是可选的。
  bool Begin(int* min_items = nullptr,
             std::vector<JumpListItem>* removed_items = nullptr);
  // 放弃自调用Begin()以来排队的所有更改。
  bool Abort();
  // 提交自调用Begin()以来排队的所有更改。
  bool Commit();
  // 删除自定义跳转列表并恢复默认跳转列表。
  bool Delete();
  // 将类别追加到自定义跳转列表。
  JumpListResult AppendCategory(const JumpListCategory& category);
  // 将类别追加到自定义跳转列表。
  JumpListResult AppendCategories(
      const std::vector<JumpListCategory>& categories);

 private:
  std::wstring app_id_;
  CComPtr<ICustomDestinationList> destinations_;

  DISALLOW_COPY_AND_ASSIGN(JumpList);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_WIN_JUMP_LIST_H_
