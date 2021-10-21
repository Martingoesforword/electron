// 版权所有(C)2015 Slake Technologies，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/electron_pdf_web_contents_helper_client.h"

ElectronPDFWebContentsHelperClient::ElectronPDFWebContentsHelperClient() =
    default;
ElectronPDFWebContentsHelperClient::~ElectronPDFWebContentsHelperClient() =
    default;

void ElectronPDFWebContentsHelperClient::UpdateContentRestrictions(
    content::WebContents* contents,
    int content_restrictions) {}
void ElectronPDFWebContentsHelperClient::OnPDFHasUnsupportedFeature(
    content::WebContents* contents) {}
void ElectronPDFWebContentsHelperClient::OnSaveURL(
    content::WebContents* contents) {}
void ElectronPDFWebContentsHelperClient::SetPluginCanSave(
    content::WebContents* contents,
    bool can_save) {}
