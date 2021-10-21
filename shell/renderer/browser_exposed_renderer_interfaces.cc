// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/renderer/browser_exposed_renderer_interfaces.h"

#include <utility>

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "build/build_config.h"
#include "electron/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "shell/renderer/renderer_client_base.h"

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "components/spellcheck/renderer/spellcheck.h"
#endif

namespace {
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
void BindSpellChecker(
    electron::RendererClientBase* client,
    mojo::PendingReceiver<spellcheck::mojom::SpellChecker> receiver) {
  if (client->GetSpellCheck())
    client->GetSpellCheck()->BindReceiver(std::move(receiver));
}
#endif

}  // 命名空间

void ExposeElectronRendererInterfacesToBrowser(
    electron::RendererClientBase* client,
    mojo::BinderMap* binders) {
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  binders->Add(base::BindRepeating(&BindSpellChecker, client),
               base::SequencedTaskRunnerHandle::Get());
#endif
}
