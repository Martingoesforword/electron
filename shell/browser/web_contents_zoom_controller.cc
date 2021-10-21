// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/web_contents_zoom_controller.h"

#include <string>

#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_type.h"
#include "net/base/url_util.h"
#include "third_party/blink/public/common/page/page_zoom.h"

namespace electron {

WebContentsZoomController::WebContentsZoomController(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  default_zoom_factor_ = kPageZoomEpsilon;
  host_zoom_map_ = content::HostZoomMap::GetForWebContents(web_contents);
}

WebContentsZoomController::~WebContentsZoomController() = default;

void WebContentsZoomController::AddObserver(
    WebContentsZoomController::Observer* observer) {
  observers_.AddObserver(observer);
}

void WebContentsZoomController::RemoveObserver(
    WebContentsZoomController::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void WebContentsZoomController::SetEmbedderZoomController(
    WebContentsZoomController* controller) {
  embedder_zoom_controller_ = controller;
}

void WebContentsZoomController::SetZoomLevel(double level) {
  if (!web_contents()->GetRenderViewHost()->IsRenderViewLive() ||
      blink::PageZoomValuesEqual(GetZoomLevel(), level) ||
      zoom_mode_ == ZoomMode::kDisabled)
    return;

  int render_process_id =
      web_contents()->GetRenderViewHost()->GetProcess()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();

  if (zoom_mode_ == ZoomMode::kManual) {
    zoom_level_ = level;

    for (Observer& observer : observers_)
      observer.OnZoomLevelChanged(web_contents(), level, true);

    return;
  }

  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  if (zoom_mode_ == ZoomMode::kIsolated ||
      zoom_map->UsesTemporaryZoomLevel(render_process_id, render_view_id)) {
    zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id, level);
    // 通知观察者缩放级别的更改。
    for (Observer& observer : observers_)
      observer.OnZoomLevelChanged(web_contents(), level, true);
  } else {
    content::HostZoomMap::SetZoomLevel(web_contents(), level);

    // 通知观察者缩放级别的更改。
    for (Observer& observer : observers_)
      observer.OnZoomLevelChanged(web_contents(), level, false);
  }
}

double WebContentsZoomController::GetZoomLevel() {
  return zoom_mode_ == ZoomMode::kManual
             ? zoom_level_
             : content::HostZoomMap::GetZoomLevel(web_contents());
}

void WebContentsZoomController::SetDefaultZoomFactor(double factor) {
  default_zoom_factor_ = factor;
}

double WebContentsZoomController::GetDefaultZoomFactor() {
  return default_zoom_factor_;
}

void WebContentsZoomController::SetTemporaryZoomLevel(double level) {
  old_process_id_ = web_contents()->GetRenderViewHost()->GetProcess()->GetID();
  old_view_id_ = web_contents()->GetRenderViewHost()->GetRoutingID();
  host_zoom_map_->SetTemporaryZoomLevel(old_process_id_, old_view_id_, level);
  // 通知观察者缩放级别的更改。
  for (Observer& observer : observers_)
    observer.OnZoomLevelChanged(web_contents(), level, true);
}

bool WebContentsZoomController::UsesTemporaryZoomLevel() {
  int render_process_id =
      web_contents()->GetRenderViewHost()->GetProcess()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();
  return host_zoom_map_->UsesTemporaryZoomLevel(render_process_id,
                                                render_view_id);
}

void WebContentsZoomController::SetZoomMode(ZoomMode new_mode) {
  if (new_mode == zoom_mode_)
    return;

  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  int render_process_id =
      web_contents()->GetRenderViewHost()->GetProcess()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();
  double original_zoom_level = GetZoomLevel();

  switch (new_mode) {
    case ZoomMode::kDefault: {
      content::NavigationEntry* entry =
          web_contents()->GetController().GetLastCommittedEntry();

      if (entry) {
        GURL url = content::HostZoomMap::GetURLFromEntry(entry);
        std::string host = net::GetHostOrSpecFromURL(url);

        if (zoom_map->HasZoomLevel(url.scheme(), host)) {
          // 如果存在同源的其他选项卡，则设置此选项卡的。
          // 缩放级别以匹配他们的缩放级别。临时缩放级别将为。
          // 下面已清除，但此调用将确保此选项卡在。
          // 正确的缩放级别。
          double origin_zoom_level =
              zoom_map->GetZoomLevelForHostAndScheme(url.scheme(), host);
          zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id,
                                          origin_zoom_level);
        } else {
          // 在删除临时标高之前，主体需要一个标高。
          // 我们不希望仅仅因为我们输入。
          // 默认模式。
          zoom_map->SetZoomLevelForHost(host, original_zoom_level);
        }
      }
      // 删除此选项卡的每个选项卡缩放数据。不需要事件回调。
      zoom_map->ClearTemporaryZoomLevel(render_process_id, render_view_id);
      break;
    }
    case ZoomMode::kIsolated: {
      // 除非在此调用之前缩放模式为|ZoomMode：：kDisabled|，否则。
      // 页面需要初始独立缩放回其所处的相同级别。
      // 在另一种模式下。
      if (zoom_mode_ != ZoomMode::kDisabled) {
        zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id,
                                        original_zoom_level);
      } else {
        // 当我们不调用任何HostZoomMap集函数时，我们发送事件。
        // 手工操作。
        for (Observer& observer : observers_)
          observer.OnZoomLevelChanged(web_contents(), original_zoom_level,
                                      false);
      }
      break;
    }
    case ZoomMode::kManual: {
      // 除非在此调用之前缩放模式为|ZoomMode：：kDisabled|，否则。
      // 需要将页面大小调整为默认缩放。当处于手动模式时，
      // 缩放级别是独立处理的。
      if (zoom_mode_ != ZoomMode::kDisabled) {
        zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id,
                                        GetDefaultZoomLevel());
        zoom_level_ = original_zoom_level;
      } else {
        // 当我们不调用任何HostZoomMap集函数时，我们发送事件。
        // 手工操作。
        for (Observer& observer : observers_)
          observer.OnZoomLevelChanged(web_contents(), original_zoom_level,
                                      false);
      }
      break;
    }
    case ZoomMode::kDisabled: {
      // 在禁用缩放之前，需要将页面缩回到默认状态。
      zoom_map->SetTemporaryZoomLevel(render_process_id, render_view_id,
                                      GetDefaultZoomLevel());
      break;
    }
  }

  zoom_mode_ = new_mode;
}

void WebContentsZoomController::ResetZoomModeOnNavigationIfNeeded(
    const GURL& url) {
  if (zoom_mode_ != ZoomMode::kIsolated && zoom_mode_ != ZoomMode::kManual)
    return;

  int render_process_id =
      web_contents()->GetRenderViewHost()->GetProcess()->GetID();
  int render_view_id = web_contents()->GetRenderViewHost()->GetRoutingID();
  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  zoom_level_ = zoom_map->GetDefaultZoomLevel();
  double new_zoom_level = zoom_map->GetZoomLevelForHostAndScheme(
      url.scheme(), net::GetHostOrSpecFromURL(url));
  for (Observer& observer : observers_)
    observer.OnZoomLevelChanged(web_contents(), new_zoom_level, false);
  zoom_map->ClearTemporaryZoomLevel(render_process_id, render_view_id);
  zoom_mode_ = ZoomMode::kDefault;
}

void WebContentsZoomController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || !navigation_handle->HasCommitted())
    return;

  if (navigation_handle->IsErrorPage()) {
    content::HostZoomMap::SendErrorPageZoomLevelRefresh(web_contents());
    return;
  }

  ResetZoomModeOnNavigationIfNeeded(navigation_handle->GetURL());
  SetZoomFactorOnNavigationIfNeeded(navigation_handle->GetURL());
}

void WebContentsZoomController::WebContentsDestroyed() {
  for (Observer& observer : observers_)
    observer.OnZoomControllerWebContentsDestroyed();

  observers_.Clear();
  embedder_zoom_controller_ = nullptr;
}

void WebContentsZoomController::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost* new_host) {
  // 如果关联的HostZoomMap发生更改，请更新我们的活动订阅。
  content::HostZoomMap* new_host_zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  if (new_host_zoom_map == host_zoom_map_)
    return;

  host_zoom_map_ = new_host_zoom_map;
}

void WebContentsZoomController::SetZoomFactorOnNavigationIfNeeded(
    const GURL& url) {
  if (blink::PageZoomValuesEqual(GetDefaultZoomFactor(), kPageZoomEpsilon))
    return;

  if (host_zoom_map_->UsesTemporaryZoomLevel(old_process_id_, old_view_id_)) {
    host_zoom_map_->ClearTemporaryZoomLevel(old_process_id_, old_view_id_);
  }

  if (embedder_zoom_controller_ &&
      embedder_zoom_controller_->UsesTemporaryZoomLevel()) {
    double level = embedder_zoom_controller_->GetZoomLevel();
    SetTemporaryZoomLevel(level);
    return;
  }

  // 当kZoomfactor可用时，它优先于。
  // 首选存储值，但如果主机显式设置了缩放因子。
  // 然后它就优先了。
  // 首选商店&lt;kZoomfactor&lt;setZoomLevel。
  std::string host = net::GetHostOrSpecFromURL(url);
  std::string scheme = url.scheme();
  double zoom_factor = GetDefaultZoomFactor();
  double zoom_level = blink::PageZoomFactorToZoomLevel(zoom_factor);
  if (host_zoom_map_->HasZoomLevel(scheme, host)) {
    zoom_level = host_zoom_map_->GetZoomLevelForHostAndScheme(scheme, host);
  }
  if (blink::PageZoomValuesEqual(zoom_level, GetZoomLevel()))
    return;

  SetZoomLevel(zoom_level);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebContentsZoomController)

}  // 命名空间电子
