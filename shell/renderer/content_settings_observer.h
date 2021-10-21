// 版权所有(C)2016 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
#define SHELL_RENDERER_CONTENT_SETTINGS_OBSERVER_H_

#include "base/compiler_specific.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"

namespace electron {

class ContentSettingsObserver : public content::RenderFrameObserver,
                                public blink::WebContentSettingsClient {
 public:
  explicit ContentSettingsObserver(content::RenderFrame* render_frame);
  ~ContentSettingsObserver() override;

  // Blink：：WebContentSettingsClient实现。
  bool AllowStorageAccessSync(StorageType storage_type) override;

 private:
  // 内容：：RenderFrameWatch实现。
  void OnDestruct() override;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsObserver);
};

}  // 命名空间电子。

#endif  // Shell_渲染器_内容_设置_观察者_H_
