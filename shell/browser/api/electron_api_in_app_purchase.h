// 版权所有(C)2017 Amaplex Software，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_API_ELECTRON_API_IN_APP_PURCHASE_H_
#define SHELL_BROWSER_API_ELECTRON_API_IN_APP_PURCHASE_H_

#include <string>
#include <vector>

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/mac/in_app_purchase.h"
#include "shell/browser/mac/in_app_purchase_observer.h"
#include "shell/browser/mac/in_app_purchase_product.h"
#include "v8/include/v8.h"

namespace electron {

namespace api {

class InAppPurchase : public gin::Wrappable<InAppPurchase>,
                      public gin_helper::EventEmitterMixin<InAppPurchase>,
                      public in_app_purchase::TransactionObserver {
 public:
  static gin::Handle<InAppPurchase> Create(v8::Isolate* isolate);

  // 杜松子酒：：可包装的。
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  InAppPurchase();
  ~InAppPurchase() override;

  v8::Local<v8::Promise> PurchaseProduct(const std::string& product_id,
                                         gin::Arguments* args);

  v8::Local<v8::Promise> GetProducts(const std::vector<std::string>& productIDs,
                                     gin::Arguments* args);

  // 事务观察者：
  void OnTransactionsUpdated(
      const std::vector<in_app_purchase::Transaction>& transactions) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(InAppPurchase);
};

}  // 命名空间API。

}  // 命名空间电子。

#endif  // SHELL_BROWSER_API_ELECTRON_API_IN_APP_PURCHASE_H_
