// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/ui/win/jump_list.h"

#include <propkey.h>  // 对于PKEY_*常量。

#include "base/cxx17_backports.h"
#include "base/logging.h"
#include "base/win/scoped_co_mem.h"
#include "base/win/scoped_propvariant.h"
#include "base/win/win_util.h"

namespace {

using electron::JumpListCategory;
using electron::JumpListItem;
using electron::JumpListResult;

bool AppendTask(const JumpListItem& item, IObjectCollection* collection) {
  DCHECK(collection);

  CComPtr<IShellLink> link;
  if (FAILED(link.CoCreateInstance(CLSID_ShellLink)) ||
      FAILED(link->SetPath(item.path.value().c_str())) ||
      FAILED(link->SetArguments(item.arguments.c_str())) ||
      FAILED(link->SetWorkingDirectory(item.working_dir.value().c_str())) ||
      FAILED(link->SetDescription(item.description.c_str())))
    return false;

  // SetDescription将参数的大小限制为INFOTIPSIZE(1024)，
  // 这意味着当超过这个限制时会被拒绝，但是实验。
  // 已经表明，超过260个字符的描述会导致静音。
  // 失败，尽管SetDescription返回成功代码S_OK。
  if (item.description.size() > 260)
    return false;

  if (!item.icon_path.empty() &&
      FAILED(link->SetIconLocation(item.icon_path.value().c_str(),
                                   item.icon_index)))
    return false;

  CComQIPtr<IPropertyStore> property_store(link);
  if (!base::win::SetStringValueForPropertyStore(property_store, PKEY_Title,
                                                 item.title.c_str()))
    return false;

  return SUCCEEDED(collection->AddObject(link));
}

bool AppendSeparator(IObjectCollection* collection) {
  DCHECK(collection);

  CComPtr<IShellLink> shell_link;
  if (SUCCEEDED(shell_link.CoCreateInstance(CLSID_ShellLink))) {
    CComQIPtr<IPropertyStore> property_store(shell_link);
    if (base::win::SetBooleanValueForPropertyStore(
            property_store, PKEY_AppUserModel_IsDestListSeparator, true))
      return SUCCEEDED(collection->AddObject(shell_link));
  }
  return false;
}

bool AppendFile(const JumpListItem& item, IObjectCollection* collection) {
  DCHECK(collection);

  CComPtr<IShellItem> file;
  if (SUCCEEDED(SHCreateItemFromParsingName(item.path.value().c_str(), NULL,
                                            IID_PPV_ARGS(&file))))
    return SUCCEEDED(collection->AddObject(file));

  return false;
}

bool GetShellItemFileName(IShellItem* shell_item, base::FilePath* file_name) {
  DCHECK(shell_item);
  DCHECK(file_name);

  base::win::ScopedCoMem<wchar_t> file_name_buffer;
  if (SUCCEEDED(
          shell_item->GetDisplayName(SIGDN_FILESYSPATH, &file_name_buffer))) {
    *file_name = base::FilePath(file_name_buffer.get());
    return true;
  }
  return false;
}

bool ConvertShellLinkToJumpListItem(IShellLink* shell_link,
                                    JumpListItem* item) {
  DCHECK(shell_link);
  DCHECK(item);

  item->type = JumpListItem::Type::kTask;
  wchar_t path[MAX_PATH];
  if (FAILED(shell_link->GetPath(path, base::size(path), nullptr, 0)))
    return false;

  item->path = base::FilePath(path);

  CComQIPtr<IPropertyStore> property_store = shell_link;
  base::win::ScopedPropVariant prop;
  if (SUCCEEDED(
          property_store->GetValue(PKEY_Link_Arguments, prop.Receive())) &&
      (prop.get().vt == VT_LPWSTR)) {
    item->arguments = prop.get().pwszVal;
  }

  if (SUCCEEDED(property_store->GetValue(PKEY_Title, prop.Receive())) &&
      (prop.get().vt == VT_LPWSTR)) {
    item->title = prop.get().pwszVal;
  }

  if (SUCCEEDED(shell_link->GetWorkingDirectory(path, base::size(path))))
    item->working_dir = base::FilePath(path);

  int icon_index;
  if (SUCCEEDED(
          shell_link->GetIconLocation(path, base::size(path), &icon_index))) {
    item->icon_path = base::FilePath(path);
    item->icon_index = icon_index;
  }

  wchar_t item_desc[INFOTIPSIZE];
  if (SUCCEEDED(shell_link->GetDescription(item_desc, base::size(item_desc))))
    item->description = item_desc;

  return true;
}

// 将IShellLink&IShellItem的IObjectArray转换为std：：Vector。
void ConvertRemovedJumpListItems(IObjectArray* in,
                                 std::vector<JumpListItem>* out) {
  DCHECK(in);
  DCHECK(out);

  UINT removed_count;
  if (SUCCEEDED(in->GetCount(&removed_count) && (removed_count > 0))) {
    out->reserve(removed_count);
    JumpListItem item;
    IShellItem* shell_item;
    IShellLink* shell_link;
    for (UINT i = 0; i < removed_count; ++i) {
      if (SUCCEEDED(in->GetAt(i, IID_PPV_ARGS(&shell_item)))) {
        item.type = JumpListItem::Type::kFile;
        GetShellItemFileName(shell_item, &item.path);
        out->push_back(item);
        shell_item->Release();
      } else if (SUCCEEDED(in->GetAt(i, IID_PPV_ARGS(&shell_link)))) {
        if (ConvertShellLinkToJumpListItem(shell_link, &item))
          out->push_back(item);
        shell_link->Release();
      }
    }
  }
}

}  // 命名空间。

namespace electron {

JumpListItem::JumpListItem() = default;
JumpListItem::JumpListItem(const JumpListItem&) = default;
JumpListItem::~JumpListItem() = default;
JumpListCategory::JumpListCategory() = default;
JumpListCategory::JumpListCategory(const JumpListCategory&) = default;
JumpListCategory::~JumpListCategory() = default;

JumpList::JumpList(const std::wstring& app_id) : app_id_(app_id) {
  destinations_.CoCreateInstance(CLSID_DestinationList);
}

JumpList::~JumpList() = default;

bool JumpList::Begin(int* min_items, std::vector<JumpListItem>* removed_items) {
  DCHECK(destinations_);
  if (!destinations_)
    return false;

  if (FAILED(destinations_->SetAppID(app_id_.c_str())))
    return false;

  UINT min_slots;
  CComPtr<IObjectArray> removed;
  if (FAILED(destinations_->BeginList(&min_slots, IID_PPV_ARGS(&removed))))
    return false;

  if (min_items)
    *min_items = min_slots;

  if (removed_items)
    ConvertRemovedJumpListItems(removed, removed_items);

  return true;
}

bool JumpList::Abort() {
  DCHECK(destinations_);
  if (!destinations_)
    return false;

  return SUCCEEDED(destinations_->AbortList());
}

bool JumpList::Commit() {
  DCHECK(destinations_);
  if (!destinations_)
    return false;

  return SUCCEEDED(destinations_->CommitList());
}

bool JumpList::Delete() {
  DCHECK(destinations_);
  if (!destinations_)
    return false;

  return SUCCEEDED(destinations_->DeleteList(app_id_.c_str()));
}

// 此方法将尝试将尽可能多的项目追加到跳转列表。
// 可能，并且将返回单个错误代码，即使有多个错误代码也是如此。
// 在这个过程中出了差错。以获取关于发生了什么的详细信息。
// 错误启用运行时日志记录。
JumpListResult JumpList::AppendCategory(const JumpListCategory& category) {
  DCHECK(destinations_);
  if (!destinations_)
    return JumpListResult::kGenericError;

  if (category.items.empty())
    return JumpListResult::kSuccess;

  CComPtr<IObjectCollection> collection;
  if (FAILED(collection.CoCreateInstance(CLSID_EnumerableObjectCollection))) {
    return JumpListResult::kGenericError;
  }

  auto result = JumpListResult::kSuccess;
  // 记录有多少项实际附加到类别中。
  int appended_count = 0;
  for (const auto& item : category.items) {
    switch (item.type) {
      case JumpListItem::Type::kTask:
        if (AppendTask(item, collection))
          ++appended_count;
        else
          LOG(ERROR) << "Failed to append task '" << item.title
                     << "' "
                        "to Jump List.";
        break;

      case JumpListItem::Type::kSeparator:
        if (category.type == JumpListCategory::Type::kTasks) {
          if (AppendSeparator(collection))
            ++appended_count;
        } else {
          LOG(ERROR) << "Can't append separator to Jump List category "
                     << "'" << category.name << "'. "
                     << "Separators are only allowed in the standard 'Tasks' "
                        "Jump List category.";
          result = JumpListResult::kCustomCategorySeparatorError;
        }
        break;

      case JumpListItem::Type::kFile:
        if (AppendFile(item, collection))
          ++appended_count;
        else
          LOG(ERROR) << "Failed to append '" << item.path.value()
                     << "' "
                        "to Jump List.";
        break;
    }
  }

  if (appended_count == 0)
    return result;

  if ((static_cast<size_t>(appended_count) < category.items.size()) &&
      (result == JumpListResult::kSuccess)) {
    result = JumpListResult::kGenericError;
  }

  CComQIPtr<IObjectArray> items(collection);

  if (category.type == JumpListCategory::Type::kTasks) {
    if (FAILED(destinations_->AddUserTasks(items))) {
      LOG(ERROR) << "Failed to append items to the standard Tasks category.";
      if (result == JumpListResult::kSuccess)
        result = JumpListResult::kGenericError;
    }
  } else {
    HRESULT hr = destinations_->AppendCategory(category.name.c_str(), items);
    if (FAILED(hr)) {
      if (hr == static_cast<HRESULT>(0x80040F03)) {
        LOG(ERROR) << "Failed to append custom category "
                   << "'" << category.name << "' "
                   << "to Jump List due to missing file type registration.";
        result = JumpListResult::kMissingFileTypeRegistrationError;
      } else if (hr == E_ACCESSDENIED) {
        LOG(ERROR) << "Failed to append custom category "
                   << "'" << category.name << "' "
                   << "to Jump List due to system privacy settings.";
        result = JumpListResult::kCustomCategoryAccessDeniedError;
      } else {
        LOG(ERROR) << "Failed to append custom category "
                   << "'" << category.name << "' to Jump List.";
        if (result == JumpListResult::kSuccess)
          result = JumpListResult::kGenericError;
      }
    }
  }
  return result;
}

// 此方法将尝试将尽可能多的类别追加到跳转列表。
// ，并将返回单个错误代码，即使有多个。
// 在这个过程中出了差错。以获取关于发生了什么的详细信息。
// 错误启用运行时日志记录。
JumpListResult JumpList::AppendCategories(
    const std::vector<JumpListCategory>& categories) {
  DCHECK(destinations_);
  if (!destinations_)
    return JumpListResult::kGenericError;

  auto result = JumpListResult::kSuccess;
  for (const auto& category : categories) {
    auto latestResult = JumpListResult::kSuccess;
    switch (category.type) {
      case JumpListCategory::Type::kTasks:
      case JumpListCategory::Type::kCustom:
        latestResult = AppendCategory(category);
        break;

      case JumpListCategory::Type::kRecent:
        if (FAILED(destinations_->AppendKnownCategory(KDC_RECENT))) {
          LOG(ERROR) << "Failed to append Recent category to Jump List.";
          latestResult = JumpListResult::kGenericError;
        }
        break;

      case JumpListCategory::Type::kFrequent:
        if (FAILED(destinations_->AppendKnownCategory(KDC_FREQUENT))) {
          LOG(ERROR) << "Failed to append Frequent category to Jump List.";
          latestResult = JumpListResult::kGenericError;
        }
        break;
    }
    // 保留第一个非泛型错误代码，因为只能从返回一个错误代码。
    // 函数(所以尽量使其成为最有用的函数)。
    if (((result == JumpListResult::kSuccess) ||
         (result == JumpListResult::kGenericError)) &&
        (latestResult != JumpListResult::kSuccess))
      result = latestResult;
  }
  return result;
}

}  // 命名空间电子
