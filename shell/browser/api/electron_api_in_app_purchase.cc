// 版权所有(C)2017 Amaplex Software，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#include "shell/browser/api/electron_api_in_app_purchase.h"

#include <string>
#include <utility>
#include <vector>

#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"

namespace gin {

template <>
struct Converter<in_app_purchase::Payment> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const in_app_purchase::Payment& payment) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.SetHidden("simple", true);
    dict.Set("productIdentifier", payment.productIdentifier);
    dict.Set("quantity", payment.quantity);
    return dict.GetHandle();
  }
};

template <>
struct Converter<in_app_purchase::Transaction> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const in_app_purchase::Transaction& val) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.SetHidden("simple", true);
    dict.Set("transactionIdentifier", val.transactionIdentifier);
    dict.Set("transactionDate", val.transactionDate);
    dict.Set("originalTransactionIdentifier",
             val.originalTransactionIdentifier);
    dict.Set("transactionState", val.transactionState);
    dict.Set("errorCode", val.errorCode);
    dict.Set("errorMessage", val.errorMessage);
    dict.Set("payment", val.payment);
    return dict.GetHandle();
  }
};

template <>
struct Converter<in_app_purchase::Product> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const in_app_purchase::Product& val) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.SetHidden("simple", true);
    dict.Set("productIdentifier", val.productIdentifier);
    dict.Set("localizedDescription", val.localizedDescription);
    dict.Set("localizedTitle", val.localizedTitle);
    dict.Set("contentVersion", val.localizedTitle);
    dict.Set("contentLengths", val.contentLengths);

    // 定价信息。
    dict.Set("price", val.price);
    dict.Set("formattedPrice", val.formattedPrice);

    // 货币信息。
    dict.Set("currencyCode", val.currencyCode);

    // 可下载的内容信息。
    dict.Set("isDownloadable", val.isDownloadable);

    return dict.GetHandle();
  }
};

}  // 命名空间杜松子酒。

namespace electron {

namespace api {

gin::WrapperInfo InAppPurchase::kWrapperInfo = {gin::kEmbedderNativeGin};

#if defined(OS_MAC)
// 静电。
gin::Handle<InAppPurchase> InAppPurchase::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new InAppPurchase());
}

gin::ObjectTemplateBuilder InAppPurchase::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<InAppPurchase>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("canMakePayments", &in_app_purchase::CanMakePayments)
      .SetMethod("restoreCompletedTransactions",
                 &in_app_purchase::RestoreCompletedTransactions)
      .SetMethod("getReceiptURL", &in_app_purchase::GetReceiptURL)
      .SetMethod("purchaseProduct", &InAppPurchase::PurchaseProduct)
      .SetMethod("finishAllTransactions",
                 &in_app_purchase::FinishAllTransactions)
      .SetMethod("finishTransactionByDate",
                 &in_app_purchase::FinishTransactionByDate)
      .SetMethod("getProducts", &InAppPurchase::GetProducts);
}

const char* InAppPurchase::GetTypeName() {
  return "InAppPurchase";
}

InAppPurchase::InAppPurchase() = default;

InAppPurchase::~InAppPurchase() = default;

v8::Local<v8::Promise> InAppPurchase::PurchaseProduct(
    const std::string& product_id,
    gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<bool> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  int quantity = 1;
  args->GetNext(&quantity);

  in_app_purchase::PurchaseProduct(
      product_id, quantity,
      base::BindOnce(gin_helper::Promise<bool>::ResolvePromise,
                     std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> InAppPurchase::GetProducts(
    const std::vector<std::string>& productIDs,
    gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<std::vector<in_app_purchase::Product>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  in_app_purchase::GetProducts(
      productIDs,
      base::BindOnce(gin_helper::Promise<
                         std::vector<in_app_purchase::Product>>::ResolvePromise,
                     std::move(promise)));

  return handle;
}

void InAppPurchase::OnTransactionsUpdated(
    const std::vector<in_app_purchase::Transaction>& transactions) {
  Emit("transactions-updated", transactions);
}
#endif

}  // 命名空间API。

}  // 命名空间电子。

namespace {

using electron::api::InAppPurchase;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
#if defined(OS_MAC)
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("inAppPurchase", InAppPurchase::Create(isolate));
#endif
}

}  // 命名空间

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_in_app_purchase, Initialize)
