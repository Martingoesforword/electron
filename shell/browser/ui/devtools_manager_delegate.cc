// 版权所有(C)2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#include "shell/browser/ui/devtools_manager_delegate.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/devtools_socket_factory.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "electron/grit/electron_resources.h"
#include "net/base/net_errors.h"
#include "net/socket/stream_socket.h"
#include "net/socket/tcp_server_socket.h"
#include "shell/browser/browser.h"
#include "shell/common/electron_paths.h"
#include "third_party/inspector_protocol/crdtp/dispatch.h"
#include "ui/base/resource/resource_bundle.h"

namespace electron {

namespace {

class TCPServerSocketFactory : public content::DevToolsSocketFactory {
 public:
  TCPServerSocketFactory(const std::string& address, int port)
      : address_(address), port_(port) {}

 private:
  // 内容：：ServerSocketFactory。
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    auto socket =
        std::make_unique<net::TCPServerSocket>(nullptr, net::NetLogSource());
    if (socket->ListenWithAddressAndPort(address_, port_, 10) != net::OK)
      return std::unique_ptr<net::ServerSocket>();

    return socket;
  }
  std::unique_ptr<net::ServerSocket> CreateForTethering(
      std::string* name) override {
    return std::unique_ptr<net::ServerSocket>();
  }

  std::string address_;
  uint16_t port_;

  DISALLOW_COPY_AND_ASSIGN(TCPServerSocketFactory);
};

std::unique_ptr<content::DevToolsSocketFactory> CreateSocketFactory() {
  auto& command_line = *base::CommandLine::ForCurrentProcess();
  // 查看用户是否在命令行上指定了端口(适用于。
  // 自动化)。如果不是，请通过指定0使用临时端口。
  int port = 0;
  if (command_line.HasSwitch(switches::kRemoteDebuggingPort)) {
    int temp_port;
    std::string port_str =
        command_line.GetSwitchValueASCII(switches::kRemoteDebuggingPort);
    if (base::StringToInt(port_str, &temp_port) && temp_port >= 0 &&
        temp_port < 65535) {
      port = temp_port;
    } else {
      DLOG(WARNING) << "Invalid http debugger port number " << temp_port;
    }
  }
  return std::make_unique<TCPServerSocketFactory>("127.0.0.1", port);
}

const char kBrowserCloseMethod[] = "Browser.close";

}  // 命名空间。

// 设备工具管理器委派-。

// 静电。
void DevToolsManagerDelegate::StartHttpHandler() {
  base::FilePath user_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_dir);
  content::DevToolsAgentHost::StartRemoteDebuggingServer(
      CreateSocketFactory(), user_dir, base::FilePath());
}

DevToolsManagerDelegate::DevToolsManagerDelegate() = default;

DevToolsManagerDelegate::~DevToolsManagerDelegate() = default;

void DevToolsManagerDelegate::Inspect(content::DevToolsAgentHost* agent_host) {}

void DevToolsManagerDelegate::HandleCommand(
    content::DevToolsAgentHostClientChannel* channel,
    base::span<const uint8_t> message,
    NotHandledCallback callback) {
  crdtp::Dispatchable dispatchable(crdtp::SpanFrom(message));
  DCHECK(dispatchable.ok());
  if (crdtp::SpanEquals(crdtp::SpanFrom(kBrowserCloseMethod),
                        dispatchable.Method())) {
    // 从理论上讲，我们应该对协议做出回应，说。
    // Browser.Close已处理。但这样做需要实例化。
    // 协议UberDispatcher并生成适当的协议处理程序。
    // 因为我们只有一种方法，而且它应该是关闭电子的，
    // 我们不需要增加这种复杂性。我们是否应该决定支持。
    // 像Browser.setWindowBound这样的方法，但是我们需要这样做。
    base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                   base::BindOnce([]() { Browser::Get()->Quit(); }));
    return;
  }
  std::move(callback).Run(message);
}

scoped_refptr<content::DevToolsAgentHost>
DevToolsManagerDelegate::CreateNewTarget(const GURL& url) {
  return nullptr;
}

std::string DevToolsManagerDelegate::GetDiscoveryPageHTML() {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
      IDR_CONTENT_SHELL_DEVTOOLS_DISCOVERY_PAGE);
}

bool DevToolsManagerDelegate::HasBundledFrontendResources() {
  return true;
}

}  // 命名空间电子
