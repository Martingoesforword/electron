// 版权所有(C)2020 Samuel Maddock&lt;Sam@samuelmaddock.com&gt;。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/extensions/electron_extension_message_filter.h"

#include <stdint.h>
#include <memory>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_handlers/default_locale_handler.h"
#include "extensions/common/manifest_handlers/shared_module_info.h"
#include "extensions/common/message_bundle.h"

using content::BrowserThread;

namespace electron {

const uint32_t kExtensionFilteredMessageClasses[] = {
    ExtensionMsgStart,
};

ElectronExtensionMessageFilter::ElectronExtensionMessageFilter(
    int render_process_id,
    content::BrowserContext* browser_context)
    : BrowserMessageFilter(kExtensionFilteredMessageClasses,
                           base::size(kExtensionFilteredMessageClasses)),
      render_process_id_(render_process_id),
      browser_context_(browser_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

ElectronExtensionMessageFilter::~ElectronExtensionMessageFilter() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

bool ElectronExtensionMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ElectronExtensionMessageFilter, message)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ExtensionHostMsg_GetMessageBundle,
                                    OnGetExtMessageBundle)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void ElectronExtensionMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message,
    BrowserThread::ID* thread) {
  switch (message.type()) {
    case ExtensionHostMsg_GetMessageBundle::ID:
      *thread = BrowserThread::UI;
      break;
    default:
      break;
  }
}

void ElectronExtensionMessageFilter::OnDestruct() const {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    delete this;
  } else {
    base::DeleteSoon(FROM_HERE, {BrowserThread::UI}, this);
  }
}

void ElectronExtensionMessageFilter::OnGetExtMessageBundle(
    const std::string& extension_id,
    IPC::Message* reply_msg) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const extensions::ExtensionSet& extension_set =
      extensions::ExtensionRegistry::Get(browser_context_)
          ->enabled_extensions();
  const extensions::Extension* extension = extension_set.GetByID(extension_id);

  if (!extension) {  // 分机已取消。
    ExtensionHostMsg_GetMessageBundle::WriteReplyParams(
        reply_msg, extensions::MessageBundle::SubstitutionMap());
    Send(reply_msg);
    return;
  }

  const std::string& default_locale =
      extensions::LocaleInfo::GetDefaultLocale(extension);
  if (default_locale.empty()) {
    // 稍加优化：将答案发送到此处，以避免额外的线程跳转。
    std::unique_ptr<extensions::MessageBundle::SubstitutionMap> dictionary_map(
        extensions::file_util::LoadNonLocalizedMessageBundleSubstitutionMap(
            extension_id));
    ExtensionHostMsg_GetMessageBundle::WriteReplyParams(reply_msg,
                                                        *dictionary_map);
    Send(reply_msg);
    return;
  }

  std::vector<base::FilePath> paths_to_load;
  paths_to_load.push_back(extension->path());

  auto imports = extensions::SharedModuleInfo::GetImports(extension);
  // 反向迭代导入。这将允许稍后导入。
  // 覆盖以前导入的模块的模块，如列表顺序所示。
  // 从导入的清单.json中的定义维护。
  for (auto it = imports.rbegin(); it != imports.rend(); ++it) {
    const extensions::Extension* imported_extension =
        extension_set.GetByID(it->extension_id);
    if (!imported_extension) {
      NOTREACHED() << "Missing shared module " << it->extension_id;
      continue;
    }
    paths_to_load.push_back(imported_extension->path());
  }

  // 这会阻止选项卡加载。优先级继承自调用上下文。
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          &ElectronExtensionMessageFilter::OnGetExtMessageBundleAsync, this,
          paths_to_load, extension_id, default_locale,
          extension_l10n_util::GetGzippedMessagesPermissionForExtension(
              extension),
          reply_msg));
}

void ElectronExtensionMessageFilter::OnGetExtMessageBundleAsync(
    const std::vector<base::FilePath>& extension_paths,
    const std::string& main_extension_id,
    const std::string& default_locale,
    extension_l10n_util::GzippedMessagesPermission gzip_permission,
    IPC::Message* reply_msg) {
  std::unique_ptr<extensions::MessageBundle::SubstitutionMap> dictionary_map(
      extensions::file_util::LoadMessageBundleSubstitutionMapFromPaths(
          extension_paths, main_extension_id, default_locale, gzip_permission));

  ExtensionHostMsg_GetMessageBundle::WriteReplyParams(reply_msg,
                                                      *dictionary_map);
  Send(reply_msg);
}

}  // 命名空间电子
