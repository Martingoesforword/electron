// 版权所有(C)2017 Amaplex Software，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_MAC_IN_APP_PURCHASE_H_
#define SHELL_BROWSER_MAC_IN_APP_PURCHASE_H_

#include <string>

#include "base/callback.h"

namespace in_app_purchase {

// 。

typedef base::OnceCallback<void(bool isProductValid)> InAppPurchaseCallback;

// 。

bool CanMakePayments();

void RestoreCompletedTransactions();

void FinishAllTransactions();

void FinishTransactionByDate(const std::string& date);

std::string GetReceiptURL();

void PurchaseProduct(const std::string& productID,
                     int quantity,
                     InAppPurchaseCallback callback);

}  // _app_purchase中的命名空间。

#endif  // Shell_Browser_MAC_IN_APP_Purchase_H_
