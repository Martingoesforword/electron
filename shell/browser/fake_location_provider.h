// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_FAKE_LOCATION_PROVIDER_H_
#define SHELL_BROWSER_FAKE_LOCATION_PROVIDER_H_

#include "base/macros.h"
#include "services/device/public/cpp/geolocation/location_provider.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace electron {

class FakeLocationProvider : public device::LocationProvider {
 public:
  FakeLocationProvider();
  ~FakeLocationProvider() override;

  // LocationProvider实现：
  void SetUpdateCallback(
      const LocationProviderUpdateCallback& callback) override;
  void StartProvider(bool high_accuracy) override;
  void StopProvider() override;
  const device::mojom::Geoposition& GetPosition() override;
  void OnPermissionGranted() override;

 private:
  device::mojom::Geoposition position_;
  LocationProviderUpdateCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(FakeLocationProvider);
};

}  // 命名空间电子。

#endif  // Shell_Browser_FAKE_LOCATION_PROVIDER_H_
