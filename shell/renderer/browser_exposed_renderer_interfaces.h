// 版权所有(C)2020 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_BROWSER_EXPOSED_RENDERER_INTERFACES_H_
#define SHELL_RENDERER_BROWSER_EXPOSED_RENDERER_INTERFACES_H_

namespace mojo {
class BinderMap;
}

namespace electron {
class RendererClientBase;
}

class ChromeContentRendererClient;

void ExposeElectronRendererInterfacesToBrowser(
    electron::RendererClientBase* client,
    mojo::BinderMap* binders);

#endif  // SHELL_RENDERER_BROWSER_EXPOSED_RENDERER_INTERFACES_H_
