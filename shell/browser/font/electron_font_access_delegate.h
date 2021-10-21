// 版权所有(C)2021 Microsoft，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_FONT_ELECTRON_FONT_ACCESS_DELEGATE_H_
#define SHELL_BROWSER_FONT_ELECTRON_FONT_ACCESS_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "content/public/browser/font_access_chooser.h"
#include "content/public/browser/font_access_delegate.h"

class ElectronFontAccessDelegate : public content::FontAccessDelegate {
 public:
  ElectronFontAccessDelegate();
  ~ElectronFontAccessDelegate() override;

  std::unique_ptr<content::FontAccessChooser> RunChooser(
      content::RenderFrameHost* frame,
      const std::vector<std::string>& selection,
      content::FontAccessChooser::Callback callback) override;

  DISALLOW_COPY_AND_ASSIGN(ElectronFontAccessDelegate);
};

#endif  // SHELL_BROWSER_FONT_ELECTRON_FONT_ACCESS_DELEGATE_H_
