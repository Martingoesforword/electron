// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_ELECTRON_NAVIGATION_THROTTLE_H_
#define SHELL_BROWSER_ELECTRON_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace electron {

class ElectronNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit ElectronNavigationThrottle(content::NavigationHandle* handle);
  ~ElectronNavigationThrottle() override;

  ElectronNavigationThrottle::ThrottleCheckResult WillStartRequest() override;

  ElectronNavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;

  const char* GetNameForLogging() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronNavigationThrottle);
};

}  // 命名空间电子。

#endif  // 外壳浏览器电子导航油门H_
