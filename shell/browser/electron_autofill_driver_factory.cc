// 版权所有(C)2019 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/electron_autofill_driver_factory.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/electron_autofill_driver.h"

namespace electron {

namespace {

std::unique_ptr<AutofillDriver> CreateDriver(
    content::RenderFrameHost* render_frame_host,
    mojom::ElectronAutofillDriverAssociatedRequest request) {
  return std::make_unique<AutofillDriver>(render_frame_host,
                                          std::move(request));
}

}  // 命名空间。

AutofillDriverFactory::~AutofillDriverFactory() = default;

// 静电。
void AutofillDriverFactory::BindAutofillDriver(
    mojom::ElectronAutofillDriverAssociatedRequest request,
    content::RenderFrameHost* render_frame_host) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return;

  AutofillDriverFactory* factory =
      AutofillDriverFactory::FromWebContents(web_contents);
  if (!factory)
    return;

  AutofillDriver* driver = factory->DriverForFrame(render_frame_host);
  if (!driver)
    factory->AddDriverForFrame(
        render_frame_host,
        base::BindOnce(CreateDriver, render_frame_host, std::move(request)));
}

AutofillDriverFactory::AutofillDriverFactory(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  const std::vector<content::RenderFrameHost*> frames =
      web_contents->GetAllFrames();
  for (content::RenderFrameHost* frame : frames) {
    if (frame->IsRenderFrameLive())
      RenderFrameCreated(frame);
  }
}

void AutofillDriverFactory::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  DeleteDriverForFrame(render_frame_host);
}

void AutofillDriverFactory::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // 就此代码而言，如果没有导航，则导航并不重要。
  // 尚未提交或是否在子帧中。
  if (!navigation_handle->HasCommitted() ||
      !navigation_handle->IsInMainFrame()) {
    return;
  }

  CloseAllPopups();
}

AutofillDriver* AutofillDriverFactory::DriverForFrame(
    content::RenderFrameHost* render_frame_host) {
  auto mapping = driver_map_.find(render_frame_host);
  return mapping == driver_map_.end() ? nullptr : mapping->second.get();
}

void AutofillDriverFactory::AddDriverForFrame(
    content::RenderFrameHost* render_frame_host,
    CreationCallback factory_method) {
  auto insertion_result =
      driver_map_.insert(std::make_pair(render_frame_host, nullptr));
  // 对于表示主框架的关键点，可以调用两次。
  if (insertion_result.second) {
    insertion_result.first->second = std::move(factory_method).Run();
  }
}

void AutofillDriverFactory::DeleteDriverForFrame(
    content::RenderFrameHost* render_frame_host) {
  driver_map_.erase(render_frame_host);
}

void AutofillDriverFactory::CloseAllPopups() {
  for (auto& it : driver_map_) {
    it.second->HideAutofillPopup();
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AutofillDriverFactory)

}  // 命名空间电子
