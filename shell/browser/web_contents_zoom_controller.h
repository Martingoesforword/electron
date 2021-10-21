// 版权所有(C)2017 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_WEB_CONTENTS_ZOOM_CONTROLLER_H_
#define SHELL_BROWSER_WEB_CONTENTS_ZOOM_CONTROLLER_H_

#include "base/observer_list_types.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace electron {

// 管理WebContents的缩放更改。
class WebContentsZoomController
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebContentsZoomController> {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnZoomLevelChanged(content::WebContents* web_contents,
                                    double level,
                                    bool is_temporary) {}
    virtual void OnZoomControllerWebContentsDestroyed() {}

   protected:
    ~Observer() override {}
  };

  // 定义如何处理缩放更改。
  enum class ZoomMode {
    // 导致默认缩放行为，即处理缩放更改。
    // 自动并基于每个原点，这意味着其他选项卡。
    // 导航到同一原点也将进行缩放。
    kDefault,
    // 导致自动处理缩放更改，但按选项卡处理。
    // 基础。中的缩放更改不会影响此缩放模式中的选项卡。
    // 其他选项卡，反之亦然。
    kIsolated,
    // 覆盖缩放更改的自动处理。OnZoomChange|onZoomChange。
    // 事件，但不会实际缩放页面。
    // 这些缩放更改可以通过侦听。
    // |onZoomChange|事件。此模式中的缩放也是基于每个选项卡的。
    kManual,
    // 禁用此选项卡中的所有缩放。该选项卡将恢复为默认值。
    // 缩放级别，所有尝试的缩放更改都将被忽略。
    kDisabled,
  };

  explicit WebContentsZoomController(content::WebContents* web_contents);
  ~WebContentsZoomController() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void SetEmbedderZoomController(WebContentsZoomController* controller);

  // 管理缩放级别的方法。
  void SetZoomLevel(double level);
  double GetZoomLevel();
  void SetDefaultZoomFactor(double factor);
  double GetDefaultZoomFactor();
  void SetTemporaryZoomLevel(double level);
  bool UsesTemporaryZoomLevel();

  // 设置缩放模式，该模式定义缩放行为(请参见枚举缩放模式)。
  void SetZoomMode(ZoomMode zoom_mode);

  void ResetZoomModeOnNavigationIfNeeded(const GURL& url);

  ZoomMode zoom_mode() const { return zoom_mode_; }

  // 获取默认缩放级别的便捷方法。在此实施的目的是。
  // 内衬。
  double GetDefaultZoomLevel() const {
    return content::HostZoomMap::GetForWebContents(web_contents())
        ->GetDefaultZoomLevel();
  }

 protected:
  // 内容：：WebContentsViewer：
  void DidFinishNavigation(content::NavigationHandle* handle) override;
  void WebContentsDestroyed() override;
  void RenderFrameHostChanged(content::RenderFrameHost* old_host,
                              content::RenderFrameHost* new_host) override;

 private:
  friend class content::WebContentsUserData<WebContentsZoomController>;

  // 在导航已提交设置默认缩放因子后调用。
  void SetZoomFactorOnNavigationIfNeeded(const GURL& url);

  // 当前的缩放模式。
  ZoomMode zoom_mode_ = ZoomMode::kDefault;

  // 当前缩放级别。
  double zoom_level_ = 1.0;

  // KZoomfactor。
  double default_zoom_factor_ = 0;

  const double kPageZoomEpsilon = 0.001;

  int old_process_id_ = -1;
  int old_view_id_ = -1;

  WebContentsZoomController* embedder_zoom_controller_ = nullptr;

  base::ObserverList<Observer> observers_;

  content::HostZoomMap* host_zoom_map_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(WebContentsZoomController);
};

}  // 命名空间电子。

#endif  // Shell_Browser_Web_Contents_ZOOM_CONTROLLER_H_
