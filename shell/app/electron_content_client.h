// 版权所有(C)2014 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_APP_ELECTRON_CONTENT_CLIENT_H_
#define SHELL_APP_ELECTRON_CONTENT_CLIENT_H_

#include <vector>

#include "base/files/file_path.h"
#include "content/public/common/content_client.h"

namespace electron {

class ElectronContentClient : public content::ContentClient {
 public:
  ElectronContentClient();
  ~ElectronContentClient() override;

 protected:
  // 内容：：ContentClient：
  std::u16string GetLocalizedString(int message_id) override;
  base::StringPiece GetDataResource(int resource_id,
                                    ui::ResourceScaleFactor) override;
  gfx::Image& GetNativeImageNamed(int resource_id) override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) override;
  void AddAdditionalSchemes(Schemes* schemes) override;
  void AddPepperPlugins(
      std::vector<content::PepperPluginInfo>* plugins) override;
  void AddContentDecryptionModules(
      std::vector<content::CdmInfo>* cdms,
      std::vector<media::CdmHostFilePath>* cdm_host_file_paths) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronContentClient);
};

}  // 命名空间电子。

#endif  // Shell_APP_ELEMENT_CONTENT_CLIENT_H_
