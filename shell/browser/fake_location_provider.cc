// 版权所有(C)2018 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/fake_location_provider.h"

#include "base/callback.h"
#include "base/time/time.h"

namespace electron {

FakeLocationProvider::FakeLocationProvider() {
  position_.latitude = 10;
  position_.longitude = -10;
  position_.accuracy = 1;
  position_.error_code =
      device::mojom::Geoposition::ErrorCode::POSITION_UNAVAILABLE;
}

FakeLocationProvider::~FakeLocationProvider() = default;

void FakeLocationProvider::SetUpdateCallback(
    const LocationProviderUpdateCallback& callback) {
  callback_ = callback;
}

void FakeLocationProvider::StartProvider(bool high_accuracy) {}

void FakeLocationProvider::StopProvider() {}

const device::mojom::Geoposition& FakeLocationProvider::GetPosition() {
  return position_;
}

void FakeLocationProvider::OnPermissionGranted() {
  if (!callback_.is_null()) {
    // 检查Device：：ValidateGeoPosition以获取值范围。
    position_.error_code = device::mojom::Geoposition::ErrorCode::NONE;
    position_.timestamp = base::Time::Now();
    callback_.Run(this, position_);
  }
}

}  // 命名空间电子
