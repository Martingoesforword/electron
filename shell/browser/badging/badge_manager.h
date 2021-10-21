// 版权所有2018年的Chromium作者。版权所有。
// 此源代码的使用受BSD样式的许可管理，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_BADGING_BADGE_MANAGER_H_
#define SHELL_BROWSER_BADGING_BADGE_MANAGER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/mojom/badging/badging.mojom.h"
#include "url/gurl.h"

namespace content {
class RenderFrameHost;
class RenderProcessHost;
}  // 命名空间内容。

namespace badging {

// 饱和之前工卡内容的最大值。
constexpr int kMaxBadgeContent = 99;

// 维护徽章内容的记录，并将徽章更改发送到。
// 委派。
class BadgeManager : public KeyedService, public blink::mojom::BadgeService {
 public:
  BadgeManager();
  ~BadgeManager() override;

  static void BindFrameReceiver(
      content::RenderFrameHost* frame,
      mojo::PendingReceiver<blink::mojom::BadgeService> receiver);
  static void BindServiceWorkerReceiver(
      content::RenderProcessHost* service_worker_process_host,
      const GURL& service_worker_scope,
      mojo::PendingReceiver<blink::mojom::BadgeService> receiver);

  // 根据一些BAGE_CONTENT确定要放在徽章上的文本。
  static std::string GetBadgeString(absl::optional<int> badge_content);

 private:
  // MOJO请求的BindingContext。允许绑定mojo调用。
  // 添加到它们所属的执行上下文，而不信任渲染器。
  // 这些信息。这是一个抽象基类，不同类型的。
  // 执行上下文派生。
  class BindingContext {
   public:
    virtual ~BindingContext() = default;
  };

  // 窗口执行上下文的BindingContext。
  class FrameBindingContext final : public BindingContext {
   public:
    FrameBindingContext(int process_id, int frame_id)
        : process_id_(process_id), frame_id_(frame_id) {}
    ~FrameBindingContext() override = default;

    int GetProcessId() { return process_id_; }
    int GetFrameId() { return frame_id_; }

   private:
    int process_id_;
    int frame_id_;
  };

  // ServiceWorkerGlobalScope执行上下文的BindingContext。
  class ServiceWorkerBindingContext final : public BindingContext {
   public:
    ServiceWorkerBindingContext(int process_id, const GURL& scope)
        : process_id_(process_id), scope_(scope) {}
    ~ServiceWorkerBindingContext() override = default;

    int GetProcessId() { return process_id_; }
    GURL GetScope() { return scope_; }

   private:
    int process_id_;
    GURL scope_;
  };

  // Blink：：mojom：：BadgeService：
  // 注意：这些是私有的，以防止它们在mojo之外被调用，因为它们。
  // 需要MOJO绑定上下文。
  void SetBadge(blink::mojom::BadgeValuePtr value) override;
  void ClearBadge() override;

  // BadgeManager的所有魔力接收器。跟踪。
  // 绑定关联的Render_Frame，因此不必依赖于。
  // 在传来它的渲染器上。
  mojo::ReceiverSet<blink::mojom::BadgeService, std::unique_ptr<BindingContext>>
      receivers_;

  // 处理徽章实际设置和清除的委托。
  // 注：此选项目前仅在Windows和MacOS上设置。
  // Std：：Unique_PTR&lt;BadgeManagerDelegate&gt;Delegate_；

  DISALLOW_COPY_AND_ASSIGN(BadgeManager);
};

}  // 命名空间标记。

#endif  // Shell_Browser_Badging_Badge_Manager_H_
