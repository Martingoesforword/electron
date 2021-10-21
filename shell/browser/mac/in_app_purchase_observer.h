// 版权所有(C)2017 Amaplex Software，Inc.。
// 此源代码的使用受麻省理工学院许可的管辖，该许可可以。
// 在许可证文件中找到。

#ifndef SHELL_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_
#define SHELL_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

#if defined(__OBJC__)
@class InAppTransactionObserver;
#else   // __对象__。
class InAppTransactionObserver;
#endif  // __对象__。

namespace in_app_purchase {

// 。

struct Payment {
  std::string productIdentifier = "";
  int quantity = 1;
};

struct Transaction {
  std::string transactionIdentifier = "";
  std::string transactionDate = "";
  std::string originalTransactionIdentifier = "";
  int errorCode = 0;
  std::string errorMessage = "";
  std::string transactionState = "";
  Payment payment;

  Transaction();
  Transaction(const Transaction&);
  ~Transaction();
};

// 。

class TransactionObserver {
 public:
  TransactionObserver();
  virtual ~TransactionObserver();

  virtual void OnTransactionsUpdated(
      const std::vector<Transaction>& transactions) = 0;

 private:
  InAppTransactionObserver* observer_;

  base::WeakPtrFactory<TransactionObserver> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(TransactionObserver);
};

}  // _app_purchase中的命名空间。

#endif  // Shell_Browser_MAC_IN_APP_Purchase_Observator_H_
