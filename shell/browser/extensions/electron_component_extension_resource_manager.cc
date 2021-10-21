// 版权所有2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/electron_component_extension_resource_manager.h"

#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/grit/component_extension_resources_map.h"
#include "electron/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "chrome/browser/pdf/pdf_extension_util.h"  // 点名检查。
#include "chrome/grit/pdf_resources_map.h"
#include "extensions/common/constants.h"
#endif

namespace extensions {

ElectronComponentExtensionResourceManager::
    ElectronComponentExtensionResourceManager() {
  AddComponentResourceEntries(kComponentExtensionResources,
                              kComponentExtensionResourcesSize);
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  AddComponentResourceEntries(kPdfResources, kPdfResourcesSize);

  // 为PDF查看器注册字符串，以便$i18n{}替换起作用。
  base::Value pdf_strings(base::Value::Type::DICTIONARY);
  pdf_extension_util::AddStrings(
      pdf_extension_util::PdfViewerContext::kPdfViewer, &pdf_strings);
  pdf_extension_util::AddAdditionalData(true, &pdf_strings);

  ui::TemplateReplacements pdf_viewer_replacements;
  ui::TemplateReplacementsFromDictionaryValue(
      base::Value::AsDictionaryValue(pdf_strings), &pdf_viewer_replacements);
  extension_template_replacements_[extension_misc::kPdfExtensionId] =
      std::move(pdf_viewer_replacements);
#endif
}

ElectronComponentExtensionResourceManager::
    ~ElectronComponentExtensionResourceManager() = default;

bool ElectronComponentExtensionResourceManager::IsComponentExtensionResource(
    const base::FilePath& extension_path,
    const base::FilePath& resource_path,
    int* resource_id) const {
  base::FilePath directory_path = extension_path;
  base::FilePath resources_dir;
  base::FilePath relative_path;
  if (!base::PathService::Get(chrome::DIR_RESOURCES, &resources_dir) ||
      !resources_dir.AppendRelativePath(directory_path, &relative_path)) {
    return false;
  }
  relative_path = relative_path.Append(resource_path);
  relative_path = relative_path.NormalizePathSeparators();

  auto entry = path_to_resource_id_.find(relative_path);
  if (entry != path_to_resource_id_.end()) {
    *resource_id = entry->second;
    return true;
  }

  return false;
}

const ui::TemplateReplacements*
ElectronComponentExtensionResourceManager::GetTemplateReplacementsForExtension(
    const std::string& extension_id) const {
  auto it = extension_template_replacements_.find(extension_id);
  if (it == extension_template_replacements_.end()) {
    return nullptr;
  }
  return &it->second;
}

void ElectronComponentExtensionResourceManager::AddComponentResourceEntries(
    const webui::ResourcePath* entries,
    size_t size) {
  base::FilePath gen_folder_path = base::FilePath().AppendASCII(
      "@out_folder@/gen/chrome/browser/resources/");
  gen_folder_path = gen_folder_path.NormalizePathSeparators();

  for (size_t i = 0; i < size; ++i) {
    base::FilePath resource_path =
        base::FilePath().AppendASCII(entries[i].path);
    resource_path = resource_path.NormalizePathSeparators();

    if (!gen_folder_path.IsParent(resource_path)) {
      DCHECK(!base::Contains(path_to_resource_id_, resource_path));
      path_to_resource_id_[resource_path] = entries[i].id;
    } else {
      // 如果资源是生成的文件，请剥离生成的文件夹的路径。
      // 以便可以从普通URL提供它(就像它不是。
      // 生成)。
      base::FilePath effective_path =
          base::FilePath().AppendASCII(resource_path.AsUTF8Unsafe().substr(
              gen_folder_path.value().length()));
      DCHECK(!base::Contains(path_to_resource_id_, effective_path));
      path_to_resource_id_[effective_path] = entries[i].id;
    }
  }
}

}  // 命名空间扩展
