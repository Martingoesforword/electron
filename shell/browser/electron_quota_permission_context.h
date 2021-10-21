// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_
#define SHELL_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_

#include "content/public/browser/quota_permission_context.h"

namespace content {
struct StorageQuotaParams;
}

namespace electron {

class ElectronQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  typedef content::QuotaPermissionContext::QuotaPermissionResponse response;

  ElectronQuotaPermissionContext();

  // 内容：：QuotaPermissionContext：
  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              PermissionCallback callback) override;

 private:
  ~ElectronQuotaPermissionContext() override;

  DISALLOW_COPY_AND_ASSIGN(ElectronQuotaPermissionContext);
};

}  // 命名空间电子。

#endif  // SHELL_BROWSER_ELECTRON_QUOTA_PERMISSION_CONTEXT_H_
