// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_VIEW_H_
#define SHELL_BROWSER_API_ELECTRON_API_VIEW_H_

#include <vector>

#include "electron/buildflags/buildflags.h"
#include "gin/handle.h"
#include "shell/common/gin_helper/wrappable.h"
#include "ui/views/view.h"

namespace electron {

namespace api {

class View : public gin_helper::Wrappable<View> {
 public:
  static gin_helper::WrappableBase* New(gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

#if BUILDFLAG(ENABLE_VIEWS_API)
  void AddChildView(gin::Handle<View> child);
  void AddChildViewAt(gin::Handle<View> child, size_t index);
#endif

  views::View* view() const { return view_; }

 protected:
  explicit View(views::View* view);
  View();
  ~View() override;

  // 应删除析构函数中的|view_|。
  void set_delete_view(bool should) { delete_view_ = should; }

 private:
  std::vector<v8::Global<v8::Object>> child_views_;

  bool delete_view_ = true;
  views::View* view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(View);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // Shell_Browser_API_Electronics_API_VIEW_H_
