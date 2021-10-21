// 版权所有2018年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/electron_extension_loader.h"

#include <utility>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"

namespace extensions {

using LoadErrorBehavior = ExtensionRegistrar::LoadErrorBehavior;

namespace {

std::pair<scoped_refptr<const Extension>, std::string> LoadUnpacked(
    const base::FilePath& extension_dir,
    int load_flags) {
  // APP_SHELL仅支持未打包的扩展。
  // 注意：如果您添加了压缩扩展支持，请考虑删除该标志。
  // 下面的Follow_SYMLINKS_Anywhere。打包的扩展不应该有符号链接。
  if (!base::DirectoryExists(extension_dir)) {
    std::string err = "Extension directory not found: " +
                      base::UTF16ToUTF8(extension_dir.LossyDisplayName());
    return std::make_pair(nullptr, err);
  }

  // Remove_Metadata文件夹。否则，将抛出以下警告。
  // 无法加载具有文件名或目录名_METADATA的扩展名。
  // 以“_”开头的文件名保留供系统使用。
  // 请参阅：https://bugs.chromium.org/p/chromium/issues/detail?id=377278。
  base::FilePath metadata_dir = extension_dir.Append(kMetadataFolder);
  if (base::DirectoryExists(metadata_dir)) {
    base::DeletePathRecursively(metadata_dir);
  }

  std::string load_error;
  scoped_refptr<Extension> extension = file_util::LoadExtension(
      extension_dir, extensions::mojom::ManifestLocation::kCommandLine,
      load_flags, &load_error);
  if (!extension.get()) {
    std::string err = "Loading extension at " +
                      base::UTF16ToUTF8(extension_dir.LossyDisplayName()) +
                      " failed with: " + load_error;
    return std::make_pair(nullptr, err);
  }

  std::string warnings;
  // 记录警告。
  if (!extension->install_warnings().empty()) {
    std::string warning_prefix =
        "Warnings loading extension at " +
        base::UTF16ToUTF8(extension_dir.LossyDisplayName());

    for (const auto& warning : extension->install_warnings()) {
      std::string unrecognized_manifest_error = ErrorUtils::FormatErrorMessage(
          manifest_errors::kUnrecognizedManifestKey, warning.key);

      if (warning.message == unrecognized_manifest_error) {
        // 筛选器kUnRecognizedManifestKey错误。此错误没有任何。
        // 影响，例如：无法识别的清单关键字‘Minimum_Chrome_Version’等。
        LOG(WARNING) << warning_prefix << ": " << warning.message;
      } else {
        warnings += "  " + warning.message + "\n";
      }
    }

    if (warnings != "") {
      warnings = warning_prefix + ":\n" + warnings;
    }
  }

  return std::make_pair(extension, warnings);
}

}  // 命名空间。

ElectronExtensionLoader::ElectronExtensionLoader(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      extension_registrar_(browser_context, this) {}

ElectronExtensionLoader::~ElectronExtensionLoader() = default;

void ElectronExtensionLoader::LoadExtension(
    const base::FilePath& extension_dir,
    int load_flags,
    base::OnceCallback<void(const Extension*, const std::string&)> cb) {
  base::PostTaskAndReplyWithResult(
      GetExtensionFileTaskRunner().get(), FROM_HERE,
      base::BindOnce(&LoadUnpacked, extension_dir, load_flags),
      base::BindOnce(&ElectronExtensionLoader::FinishExtensionLoad,
                     weak_factory_.GetWeakPtr(), std::move(cb)));
}

void ElectronExtensionLoader::ReloadExtension(const ExtensionId& extension_id) {
  const Extension* extension = ExtensionRegistry::Get(browser_context_)
                                   ->GetInstalledExtension(extension_id);
  // 我们不应该尝试重新加载尚未添加的扩展。
  DCHECK(extension);

  // 这应该总是从false开始，因为它只在这里设置，或者在。
  // LoadExtensionForReload()作为以下调用的结果。
  DCHECK_EQ(false, did_schedule_reload_);
  base::AutoReset<bool> reset_did_schedule_reload(&did_schedule_reload_, false);

  extension_registrar_.ReloadExtension(extension_id, LoadErrorBehavior::kQuiet);
  if (did_schedule_reload_)
    return;
}

void ElectronExtensionLoader::UnloadExtension(
    const ExtensionId& extension_id,
    extensions::UnloadedExtensionReason reason) {
  extension_registrar_.RemoveExtension(extension_id, reason);
}

void ElectronExtensionLoader::FinishExtensionLoad(
    base::OnceCallback<void(const Extension*, const std::string&)> cb,
    std::pair<scoped_refptr<const Extension>, std::string> result) {
  scoped_refptr<const Extension> extension = result.first;
  if (extension) {
    extension_registrar_.AddExtension(extension);

    // 将扩展安装时间写入ExtensionPrefs。这是以下条件所必需的。
    // 调用Extensions：：ExtensionPrefs：：GetInstallTime的WebRequestAPI。
    // 
    // 编写首选项的实现基于。
    // PreferenceAPIBase：：SetExtensionControlledPref.。
    {
      ExtensionPrefs* extension_prefs = ExtensionPrefs::Get(browser_context_);
      ExtensionPrefs::ScopedDictionaryUpdate update(
          extension_prefs, extension.get()->id(),
          extensions::pref_names::kPrefPreferences);
      auto preference = update.Create();
      const base::Time install_time = base::Time::Now();
      preference->SetString(
          "install_time", base::NumberToString(install_time.ToInternalValue()));
    }
  }

  std::move(cb).Run(extension.get(), result.second);
}

void ElectronExtensionLoader::FinishExtensionReload(
    const ExtensionId& old_extension_id,
    std::pair<scoped_refptr<const Extension>, std::string> result) {
  scoped_refptr<const Extension> extension = result.first;
  if (extension) {
    extension_registrar_.AddExtension(extension);
  }
}

void ElectronExtensionLoader::PreAddExtension(const Extension* extension,
                                              const Extension* old_extension) {
  if (old_extension)
    return;

  // 如果上一次重新加载尝试失败，则可能会禁用该扩展。在……里面。
  // 在这种情况下，我们希望删除禁用原因。
  ExtensionPrefs* extension_prefs = ExtensionPrefs::Get(browser_context_);
  if (extension_prefs->IsExtensionDisabled(extension->id()) &&
      extension_prefs->HasDisableReason(extension->id(),
                                        disable_reason::DISABLE_RELOAD)) {
    extension_prefs->RemoveDisableReason(extension->id(),
                                         disable_reason::DISABLE_RELOAD);
    // 只有在没有其他禁用原因时才重新启用扩展。
    if (extension_prefs->GetDisableReasons(extension->id()) ==
        disable_reason::DISABLE_NONE) {
      extension_prefs->SetExtensionEnabled(extension->id());
    }
  }
}

void ElectronExtensionLoader::PostActivateExtension(
    scoped_refptr<const Extension> extension) {}

void ElectronExtensionLoader::PostDeactivateExtension(
    scoped_refptr<const Extension> extension) {}

void ElectronExtensionLoader::LoadExtensionForReload(
    const ExtensionId& extension_id,
    const base::FilePath& path,
    LoadErrorBehavior load_error_behavior) {
  CHECK(!path.empty());

  // TODO(Nornagon)：我们应该保存是否授予了文件访问权限。
  // 加载此扩展时，请将其保留在此处。按照原样，重新加载。
  // 扩展名将导致文件访问权限被删除。
  int load_flags = Extension::FOLLOW_SYMLINKS_ANYWHERE;
  base::PostTaskAndReplyWithResult(
      GetExtensionFileTaskRunner().get(), FROM_HERE,
      base::BindOnce(&LoadUnpacked, path, load_flags),
      base::BindOnce(&ElectronExtensionLoader::FinishExtensionReload,
                     weak_factory_.GetWeakPtr(), extension_id));
  did_schedule_reload_ = true;
}

bool ElectronExtensionLoader::CanEnableExtension(const Extension* extension) {
  return true;
}

bool ElectronExtensionLoader::CanDisableExtension(const Extension* extension) {
  // 用户无法禁用分机。
  return false;
}

bool ElectronExtensionLoader::ShouldBlockExtension(const Extension* extension) {
  return false;
}

}  // 命名空间扩展
