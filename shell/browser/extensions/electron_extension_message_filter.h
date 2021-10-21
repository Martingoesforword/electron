// 版权所有(C)2020 Samuel Maddock&lt;Sam@samuelmaddock.com&gt;。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_MESSAGE_FILTER_H_
#define SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/sequenced_task_runner_helpers.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/extension_l10n_util.h"

namespace content {
class BrowserContext;
}

namespace extensions {
struct Message;
}

namespace electron {

// 类过滤掉传入的特定于电子的IPC消息。
// IPC线程上的扩展进程。
class ElectronExtensionMessageFilter : public content::BrowserMessageFilter {
 public:
  ElectronExtensionMessageFilter(int render_process_id,
                                 content::BrowserContext* browser_context);

  // 内容：：BrowserMessageFilter方法：
  bool OnMessageReceived(const IPC::Message& message) override;
  void OverrideThreadForMessage(const IPC::Message& message,
                                content::BrowserThread::ID* thread) override;
  void OnDestruct() const override;

 private:
  friend class content::BrowserThread;
  friend class base::DeleteHelper<ElectronExtensionMessageFilter>;

  ~ElectronExtensionMessageFilter() override;

  void OnGetExtMessageBundle(const std::string& extension_id,
                             IPC::Message* reply_msg);
  void OnGetExtMessageBundleAsync(
      const std::vector<base::FilePath>& extension_paths,
      const std::string& main_extension_id,
      const std::string& default_locale,
      extension_l10n_util::GzippedMessagesPermission gzip_permission,
      IPC::Message* reply_msg);

  const int render_process_id_;

  // 与我们的呈现器进程相关联的BrowserContext。这应该只是。
  // 可以在UI线程上访问！此外，由于这一类是重新计算的，所以。
  // 可能比|BROWSER_CONTEXT_|更长，因此如果有疑问，请确保检查为空；
  // 异步呼叫等。
  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(ElectronExtensionMessageFilter);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_MESSAGE_FILTER_H_
