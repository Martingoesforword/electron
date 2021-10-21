import { closeWindow } from './window-helpers';

import { BaseWindow, WebContentsView } from 'electron/main';

describe('WebContentsView', () => {
  let w: BaseWindow;
  afterEach(() => closeWindow(w as any).then(() => { w = null as unknown as BaseWindow; }));

  it('can be used as content view', () => {
    w = new BaseWindow({ show: false });
    w.setContentView(new WebContentsView({}));
  });

  function triggerGCByAllocation () {
    const arr = [];
    for (let i = 0; i < 1000000; i++) {
      arr.push([]);
    }
    return arr;
  }

  it('doesn\'t crash when GCed during allocation', (done) => {
    // Eslint-Disable-Next-Next-new。
    new WebContentsView({});
    setTimeout(() => {
      // 注意：我们正在测试的崩溃是缺少当前的`v8：：Context`。
      // 在WebContents的析构函数中发出事件时。V8不一致。
      // 关于垃圾处理期间是否存在当前上下文。
      // 集合，似乎`v8Util.requestGarbageCollectionForTesting`。
      // 导致一个GC，其中有一个当前上下文，所以崩溃不是。
      // 触发了。因此，我们通过其他方式强制GC：即，通过分配。
      // 一堆东西。
      triggerGCByAllocation();
      done();
    });
  });
});
