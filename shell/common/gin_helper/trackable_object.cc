// 版权所有(C)2015 GitHub，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/common/gin_helper/trackable_object.h"

#include <memory>

#include "base/bind.h"
#include "base/supports_user_data.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/common/gin_helper/locker.h"

namespace gin_helper {

namespace {

const char kTrackedObjectKey[] = "TrackedObjectKey";

class IDUserData : public base::SupportsUserData::Data {
 public:
  explicit IDUserData(int32_t id) : id_(id) {}

  operator int32_t() const { return id_; }

 private:
  int32_t id_;

  DISALLOW_COPY_AND_ASSIGN(IDUserData);
};

}  // 命名空间。

TrackableObjectBase::TrackableObjectBase() {
  // TODO(Zcbenz)：使TrackedObject在渲染器进程中工作。
  DCHECK(gin_helper::Locker::IsBrowserProcess())
      << "This class only works for browser process";
}

TrackableObjectBase::~TrackableObjectBase() = default;

base::OnceClosure TrackableObjectBase::GetDestroyClosure() {
  return base::BindOnce(&TrackableObjectBase::Destroy,
                        weak_factory_.GetWeakPtr());
}

void TrackableObjectBase::Destroy() {
  delete this;
}

void TrackableObjectBase::AttachAsUserData(base::SupportsUserData* wrapped) {
  wrapped->SetUserData(kTrackedObjectKey,
                       std::make_unique<IDUserData>(weak_map_id_));
}

// 静电。
int32_t TrackableObjectBase::GetIDFromWrappedClass(
    base::SupportsUserData* wrapped) {
  if (wrapped) {
    auto* id =
        static_cast<IDUserData*>(wrapped->GetUserData(kTrackedObjectKey));
    if (id)
      return *id;
  }
  return 0;
}

}  // 命名空间gin_helper
