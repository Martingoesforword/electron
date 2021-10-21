// 版权所有(C)2021 Microsoft，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/hid/electron_hid_delegate.h"

#include <string>
#include <utility>

#include "content/public/browser/web_contents.h"
#include "shell/browser/hid/hid_chooser_context.h"
#include "shell/browser/hid/hid_chooser_context_factory.h"
#include "shell/browser/hid/hid_chooser_controller.h"
#include "shell/browser/web_contents_permission_helper.h"

namespace {

electron::HidChooserContext* GetChooserContext(
    content::RenderFrameHost* frame) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(frame);
  auto* browser_context = web_contents->GetBrowserContext();
  return electron::HidChooserContextFactory::GetForBrowserContext(
      browser_context);
}

}  // 命名空间。

namespace electron {

ElectronHidDelegate::ElectronHidDelegate() = default;

ElectronHidDelegate::~ElectronHidDelegate() = default;

std::unique_ptr<content::HidChooser> ElectronHidDelegate::RunChooser(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::HidDeviceFilterPtr> filters,
    content::HidChooser::Callback callback) {
  electron::HidChooserContext* chooser_context =
      GetChooserContext(render_frame_host);
  if (!device_observation_.IsObserving())
    device_observation_.Observe(chooser_context);

  HidChooserController* controller = ControllerForFrame(render_frame_host);
  if (controller) {
    DeleteControllerForFrame(render_frame_host);
  }
  AddControllerForFrame(render_frame_host, std::move(filters),
                        std::move(callback));

  // 返回nullptr，因为返回值没有任何用途，例如。
  // 没有取消Navigator.id.requestDevice()的机制。回报。
  // 值只是在Chromium中用来清理选择器UI，一旦序列。
  // 服务被毁了。
  return nullptr;
}

bool ElectronHidDelegate::CanRequestDevicePermission(
    content::RenderFrameHost* render_frame_host) {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  return permission_helper->CheckHIDAccessPermission(
      web_contents->GetMainFrame()->GetLastCommittedOrigin());
}

bool ElectronHidDelegate::HasDevicePermission(
    content::RenderFrameHost* render_frame_host,
    const device::mojom::HidDeviceInfo& device) {
  auto* chooser_context = GetChooserContext(render_frame_host);
  const auto& origin =
      render_frame_host->GetMainFrame()->GetLastCommittedOrigin();
  return chooser_context->HasDevicePermission(origin, device,
                                              render_frame_host);
}

device::mojom::HidManager* ElectronHidDelegate::GetHidManager(
    content::RenderFrameHost* render_frame_host) {
  auto* chooser_context = GetChooserContext(render_frame_host);
  return chooser_context->GetHidManager();
}

void ElectronHidDelegate::AddObserver(
    content::RenderFrameHost* render_frame_host,
    Observer* observer) {
  observer_list_.AddObserver(observer);
  auto* chooser_context = GetChooserContext(render_frame_host);
  if (!device_observation_.IsObserving())
    device_observation_.Observe(chooser_context);
}

void ElectronHidDelegate::RemoveObserver(
    content::RenderFrameHost* render_frame_host,
    content::HidDelegate::Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

const device::mojom::HidDeviceInfo* ElectronHidDelegate::GetDeviceInfo(
    content::RenderFrameHost* render_frame_host,
    const std::string& guid) {
  auto* chooser_context = GetChooserContext(render_frame_host);
  return chooser_context->GetDeviceInfo(guid);
}

bool ElectronHidDelegate::IsFidoAllowedForOrigin(const url::Origin& origin) {
  return false;
}

void ElectronHidDelegate::OnDeviceAdded(
    const device::mojom::HidDeviceInfo& device_info) {
  for (auto& observer : observer_list_)
    observer.OnDeviceAdded(device_info);
}

void ElectronHidDelegate::OnDeviceRemoved(
    const device::mojom::HidDeviceInfo& device_info) {
  for (auto& observer : observer_list_)
    observer.OnDeviceRemoved(device_info);
}

void ElectronHidDelegate::OnDeviceChanged(
    const device::mojom::HidDeviceInfo& device_info) {
  for (auto& observer : observer_list_)
    observer.OnDeviceChanged(device_info);
}

void ElectronHidDelegate::OnHidManagerConnectionError() {
  device_observation_.Reset();

  for (auto& observer : observer_list_)
    observer.OnHidManagerConnectionError();
}

void ElectronHidDelegate::OnHidChooserContextShutdown() {
  device_observation_.Reset();
}

HidChooserController* ElectronHidDelegate::ControllerForFrame(
    content::RenderFrameHost* render_frame_host) {
  auto mapping = controller_map_.find(render_frame_host);
  return mapping == controller_map_.end() ? nullptr : mapping->second.get();
}

HidChooserController* ElectronHidDelegate::AddControllerForFrame(
    content::RenderFrameHost* render_frame_host,
    std::vector<blink::mojom::HidDeviceFilterPtr> filters,
    content::HidChooser::Callback callback) {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto controller = std::make_unique<HidChooserController>(
      render_frame_host, std::move(filters), std::move(callback), web_contents,
      weak_factory_.GetWeakPtr());
  controller_map_.insert(
      std::make_pair(render_frame_host, std::move(controller)));
  return ControllerForFrame(render_frame_host);
}

void ElectronHidDelegate::DeleteControllerForFrame(
    content::RenderFrameHost* render_frame_host) {
  controller_map_.erase(render_frame_host);
}

}  // 命名空间电子
