// 版权所有(C)2020 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_VIEWS_ELECTRON_API_IMAGE_VIEW_H_
#define SHELL_BROWSER_API_VIEWS_ELECTRON_API_IMAGE_VIEW_H_

#include "gin/handle.h"
#include "shell/browser/api/electron_api_view.h"
#include "ui/gfx/image/image.h"
#include "ui/views/controls/image_view.h"

namespace electron {

namespace api {

class ImageView : public View {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void SetImage(const gfx::Image& image);

 protected:
  ImageView();
  ~ImageView() override;

  views::ImageView* image_view() const {
    return static_cast<views::ImageView*>(view());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ImageView);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_VIEWS_ELECTRON_API_IMAGE_VIEW_H_
