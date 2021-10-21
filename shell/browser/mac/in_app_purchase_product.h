// 版权所有(C)2018 Amaplex Software，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_MAC_IN_APP_PURCHASE_PRODUCT_H_
#define SHELL_BROWSER_MAC_IN_APP_PURCHASE_PRODUCT_H_

#include <string>
#include <vector>

#include "base/callback.h"

namespace in_app_purchase {

// 。

struct Product {
  // 产品标识符。
  std::string productIdentifier;

  // 产品属性。
  std::string localizedDescription;
  std::string localizedTitle;
  std::string contentVersion;
  std::vector<uint32_t> contentLengths;

  // 定价信息。
  double price = 0.0;
  std::string formattedPrice;

  // 货币信息。
  std::string currencyCode;

  // 可下载的内容信息。
  bool isDownloadable = false;

  Product(const Product&);
  Product();
  ~Product();
};

// 。

typedef base::OnceCallback<void(std::vector<in_app_purchase::Product>)>
    InAppPurchaseProductsCallback;

// 。

void GetProducts(const std::vector<std::string>& productIDs,
                 InAppPurchaseProductsCallback callback);

}  // _app_purchase中的命名空间。

#endif  // Shell_Browser_MAC_IN_APP_Purchase_PRODUCT_H_
