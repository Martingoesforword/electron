import { expect } from 'chai';
import { inAppPurchase } from 'electron/main';

describe('inAppPurchase module', function () {
  if (process.platform !== 'darwin') return;

  this.timeout(3 * 60 * 1000);

  it('canMakePayments() returns a boolean', () => {
    const canMakePayments = inAppPurchase.canMakePayments();
    expect(canMakePayments).to.be.a('boolean');
  });

  it('restoreCompletedTransactions() does not throw', () => {
    expect(() => {
      inAppPurchase.restoreCompletedTransactions();
    }).to.not.throw();
  });

  it('finishAllTransactions() does not throw', () => {
    expect(() => {
      inAppPurchase.finishAllTransactions();
    }).to.not.throw();
  });

  it('finishTransactionByDate() does not throw', () => {
    expect(() => {
      inAppPurchase.finishTransactionByDate(new Date().toISOString());
    }).to.not.throw();
  });

  it('getReceiptURL() returns receipt URL', () => {
    expect(inAppPurchase.getReceiptURL()).to.match(/_MASReceipt\/receipt$/);
  });

  // 以下三个测试被禁用，因为它们击中了Apple服务器，并且。
  // 苹果开始屏蔽来自AWS IP的请求(我们认为)，所以他们在CI上失败了。
  // TODO：找到一种方法来模拟服务器请求，这样我们就可以测试这些API。
  // 而不依赖于远程服务。
  xdescribe('handles product purchases', () => {
    it('purchaseProduct() fails when buying invalid product', async () => {
      const success = await inAppPurchase.purchaseProduct('non-exist', 1);
      expect(success).to.be.false('failed to purchase non-existent product');
    });

    it('purchaseProduct() accepts optional arguments', async () => {
      const success = await inAppPurchase.purchaseProduct('non-exist');
      expect(success).to.be.false('failed to purchase non-existent product');
    });

    it('getProducts() returns an empty list when getting invalid product', async () => {
      const products = await inAppPurchase.getProducts(['non-exist']);
      expect(products).to.be.an('array').of.length(0);
    });
  });
});
