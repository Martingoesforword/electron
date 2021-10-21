const { expect } = require('chai');
const { webFrame } = require('electron');

describe('webFrame module', function () {
  it('top is self for top frame', () => {
    expect(webFrame.top.context).to.equal(webFrame.context);
  });

  it('opener is null for top frame', () => {
    expect(webFrame.opener).to.be.null();
  });

  it('firstChild is null for top frame', () => {
    expect(webFrame.firstChild).to.be.null();
  });

  it('getFrameForSelector() does not crash when not found', () => {
    expect(webFrame.getFrameForSelector('unexist-selector')).to.be.null();
  });

  it('findFrameByName() does not crash when not found', () => {
    expect(webFrame.findFrameByName('unexist-name')).to.be.null();
  });

  it('findFrameByRoutingId() does not crash when not found', () => {
    expect(webFrame.findFrameByRoutingId(-1)).to.be.null();
  });

  describe('executeJavaScript', () => {
    let childFrameElement, childFrame;

    before(() => {
      childFrameElement = document.createElement('iframe');
      document.body.appendChild(childFrameElement);
      childFrame = webFrame.firstChild;
    });

    after(() => {
      childFrameElement.remove();
    });

    it('executeJavaScript() yields results via a promise and a sync callback', async () => {
      let callbackResult, callbackError;

      const executeJavaScript = childFrame
        .executeJavaScript('1 + 1', (result, error) => {
          callbackResult = result;
          callbackError = error;
        });

      expect(callbackResult).to.equal(2);
      expect(callbackError).to.be.undefined();

      const promiseResult = await executeJavaScript;
      expect(promiseResult).to.equal(2);
    });

    it('executeJavaScriptInIsolatedWorld() yields results via a promise and a sync callback', async () => {
      let callbackResult, callbackError;

      const executeJavaScriptInIsolatedWorld = childFrame
        .executeJavaScriptInIsolatedWorld(999, [{ code: '1 + 1' }], (result, error) => {
          callbackResult = result;
          callbackError = error;
        });

      expect(callbackResult).to.equal(2);
      expect(callbackError).to.be.undefined();

      const promiseResult = await executeJavaScriptInIsolatedWorld;
      expect(promiseResult).to.equal(2);
    });

    it('executeJavaScript() yields errors via a promise and a sync callback', async () => {
      let callbackResult, callbackError;

      const executeJavaScript = childFrame
        .executeJavaScript('thisShouldProduceAnError()', (result, error) => {
          callbackResult = result;
          callbackError = error;
        });

      expect(callbackResult).to.be.undefined();
      expect(callbackError).to.be.an('error');

      await expect(executeJavaScript).to.eventually.be.rejected('error is expected');
    });

    // EcuteJavaScriptInIsolatedWorld检测不到EXEC错误。
    // 拒绝或将错误传递给回调。这在重新引入之前就已经存在了。
    // 因此不会作为回调PR的一部分进行修复。
    // 如果/当此问题得到修复时，可以取消对测试的注释。
    // 
    // It(‘ecuteJavaScriptInIsolatedWorld()通过承诺和同步回调产生错误’，Done=&gt;{。
    // 让callbackResult，callbackError。
    // 
    // Const ecuteJavaScriptInIsolatedWorld=Child Frame。
    // .ecuteJavaScriptInIsolatedWorld(999，[{code：‘thisShouldProduceAnError()’}]，(Result，Error)=&gt;{。
    // CallbackResult=结果。
    // CallbackError=错误。
    // })；
    // 
    // Expect(CallbackResult).to.be.unfined()。
    // Expect(CallbackError).to.be.an(‘Error’)。
    // 
    // 预期为expect(executeJavaScriptInIsolatedWorld).to.eventually.be.rejected(‘error’)；
    // })

    it('executeJavaScript(InIsolatedWorld) can be used without a callback', async () => {
      expect(await webFrame.executeJavaScript('1 + 1')).to.equal(2);
      expect(await webFrame.executeJavaScriptInIsolatedWorld(999, [{ code: '1 + 1' }])).to.equal(2);
    });
  });
});
