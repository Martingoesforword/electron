// 版权所有(C)2015 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。
#ifndef SHELL_BROWSER_ELECTRON_PDF_WEB_CONTENTS_HELPER_CLIENT_H_
#define SHELL_BROWSER_ELECTRON_PDF_WEB_CONTENTS_HELPER_CLIENT_H_

#include "components/pdf/browser/pdf_web_contents_helper_client.h"

namespace content {
class WebContents;
}

class ElectronPDFWebContentsHelperClient
    : public pdf::PDFWebContentsHelperClient {
 public:
  ElectronPDFWebContentsHelperClient();
  ~ElectronPDFWebContentsHelperClient() override;

 private:
  // PDF：：PDFWebContentsHelperClient。
  void UpdateContentRestrictions(content::WebContents* contents,
                                 int content_restrictions) override;
  void OnPDFHasUnsupportedFeature(content::WebContents* contents) override;
  void OnSaveURL(content::WebContents* contents) override;
  void SetPluginCanSave(content::WebContents* contents, bool can_save) override;
};

#endif  // SHELL_BROWSER_ELECTRON_PDF_WEB_CONTENTS_HELPER_CLIENT_H_
