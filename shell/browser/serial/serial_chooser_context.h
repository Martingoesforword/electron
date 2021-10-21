// 版权所有2019年Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_H_
#define SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/unguessable_token.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/serial_delegate.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/device/public/mojom/serial.mojom-forward.h"
#include "shell/browser/electron_browser_context.h"
#include "third_party/blink/public/mojom/serial/serial.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace base {
class Value;
}

namespace electron {

extern const char kHidVendorIdKey[];
extern const char kHidProductIdKey[];

#if defined(OS_WIN)
extern const char kDeviceInstanceIdKey[];
#else
extern const char kVendorIdKey[];
extern const char kProductIdKey[];
extern const char kSerialNumberKey[];
#if defined(OS_MAC)
extern const char kUsbDriverKey[];
#endif  // 已定义(OS_MAC)。
#endif  // 已定义(OS_WIN)。

class SerialChooserContext : public KeyedService,
                             public device::mojom::SerialPortManagerClient {
 public:
  using PortObserver = content::SerialDelegate::Observer;

  SerialChooserContext();
  ~SerialChooserContext() override;

  // 用于授予和检查权限的串行特定接口。
  void GrantPortPermission(const url::Origin& origin,
                           const device::mojom::SerialPortInfo& port,
                           content::RenderFrameHost* render_frame_host);
  bool HasPortPermission(const url::Origin& origin,
                         const device::mojom::SerialPortInfo& port,
                         content::RenderFrameHost* render_frame_host);
  static bool CanStorePersistentEntry(
      const device::mojom::SerialPortInfo& port);

  device::mojom::SerialPortManager* GetPortManager();

  void AddPortObserver(PortObserver* observer);
  void RemovePortObserver(PortObserver* observer);

  base::WeakPtr<SerialChooserContext> AsWeakPtr();

  // SerialPortManagerClient实现。
  void OnPortAdded(device::mojom::SerialPortInfoPtr port) override;
  void OnPortRemoved(device::mojom::SerialPortInfoPtr port) override;

 private:
  void EnsurePortManagerConnection();
  void SetUpPortManagerConnection(
      mojo::PendingRemote<device::mojom::SerialPortManager> manager);
  void OnPortManagerConnectionError();

  // 跟踪原点有权访问的一组端口。
  std::map<url::Origin, std::set<base::UnguessableToken>> ephemeral_ports_;

  // 保存有关|EMEMPERAL_PORTS_|中的端口的信息。
  std::map<base::UnguessableToken, base::Value> port_info_;

  mojo::Remote<device::mojom::SerialPortManager> port_manager_;
  mojo::Receiver<device::mojom::SerialPortManagerClient> client_receiver_{this};
  base::ObserverList<PortObserver> port_observer_list_;

  base::WeakPtrFactory<SerialChooserContext> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(SerialChooserContext);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_SERIAL_SERIAL_CHOOSER_CONTEXT_H_
