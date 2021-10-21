// 版权所有(C)2014年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证铬档案里找到的。

#ifndef SHELL_BROWSER_UI_VIEWS_ELECTRON_VIEWS_DELEGATE_H_
#define SHELL_BROWSER_UI_VIEWS_ELECTRON_VIEWS_DELEGATE_H_

#include <map>
#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "ui/views/views_delegate.h"

namespace electron {

class ViewsDelegate : public views::ViewsDelegate {
 public:
  ViewsDelegate();
  ~ViewsDelegate() override;

 protected:
  // 视图：：视图委派：
  void SaveWindowPlacement(const views::Widget* window,
                           const std::string& window_name,
                           const gfx::Rect& bounds,
                           ui::WindowShowState show_state) override;
  bool GetSavedWindowPlacement(const views::Widget* widget,
                               const std::string& window_name,
                               gfx::Rect* bounds,
                               ui::WindowShowState* show_state) const override;
  void NotifyMenuItemFocused(const std::u16string& menu_name,
                             const std::u16string& menu_item_name,
                             int item_index,
                             int item_count,
                             bool has_submenu) override;

#if defined(OS_WIN)
  HICON GetDefaultWindowIcon() const override;
  HICON GetSmallWindowIcon() const override;
  bool IsWindowInMetro(gfx::NativeWindow window) const override;
  int GetAppbarAutohideEdges(HMONITOR monitor,
                             base::OnceClosure callback) override;
#elif defined(OS_LINUX) && !defined(OS_CHROMEOS)
  gfx::ImageSkia* GetDefaultWindowIcon() const override;
#endif
  std::unique_ptr<views::NonClientFrameView> CreateDefaultNonClientFrameView(
      views::Widget* widget) override;
  void AddRef() override;
  void ReleaseRef() override;
  void OnBeforeWidgetInit(
      views::Widget::InitParams* params,
      views::internal::NativeWidgetDelegate* delegate) override;
  bool WindowManagerProvidesTitleBar(bool maximized) override;

 private:
#if defined(OS_WIN)
  using AppbarAutohideEdgeMap = std::map<HMONITOR, int>;

  // 带边缘的主线程上的回调。|RETURNED_EDGES|是返回的值。
  // 从启动查找的GetAutohideEdges()调用返回。
  void OnGotAppbarAutohideEdges(base::OnceClosure callback,
                                HMONITOR monitor,
                                int returned_edges,
                                int edges);

  AppbarAutohideEdgeMap appbar_autohide_edge_map_;
  // 如果为True，则我们正在通知来自。
  // GetAutohideEdges()。启动新查询。
  bool in_autohide_edges_callback_ = false;

  base::WeakPtrFactory<ViewsDelegate> weak_factory_{this};
#endif

  DISALLOW_COPY_AND_ASSIGN(ViewsDelegate);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_UI_VIEWS_ELECTRON_VIEWS_DELEGATE_H_
