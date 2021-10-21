// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_
#define SHELL_BROWSER_UI_COCOA_VIEWS_DELEGATE_MAC_H_

#include "ui/views/views_delegate.h"

namespace electron {

class ViewsDelegateMac : public views::ViewsDelegate {
 public:
  ViewsDelegateMac();
  ~ViewsDelegateMac() override;

  // 视图委派：
  void OnBeforeWidgetInit(
      views::Widget::InitParams* params,
      views::internal::NativeWidgetDelegate* delegate) override;
  ui::ContextFactory* GetContextFactory() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ViewsDelegateMac);
};

}  // 命名空间电子。

#endif  // Shell_Browser_UI_CoCoA_Views_Delegate_MAC_H_
