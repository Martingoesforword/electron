import * as util from 'util';

const timers = require('timers');

type AnyFn = (...args: any[]) => any

// SetImmediate和process.nextTick利用UV_Check和UV_Prepare。
// 运行回调，但是，因为我们只对请求运行UV循环，所以。
// 在其他东西激活UV循环之前，不会调用回调，
// 这将使回调延迟任意长时间。所以我们应该。
// 在setImmediate和process之后主动激活UV循环。nextTick是。
// 打过电话。
const wrapWithActivateUvLoop = function <T extends AnyFn> (func: T): T {
  return wrap(func, function (func) {
    return function (this: any, ...args: any[]) {
      process.activateUvLoop();
      return func.apply(this, args);
    };
  }) as T;
};

/* **对函数的以下任何强制转换是由于打字脚本不支持索引签名中的符号**参考：https://github.com/Microsoft/TypeScript/issues/1863。*/
function wrap <T extends AnyFn> (func: T, wrapper: (fn: AnyFn) => T) {
  const wrapped = wrapper(func);
  if ((func as any)[util.promisify.custom]) {
    (wrapped as any)[util.promisify.custom] = wrapper((func as any)[util.promisify.custom]);
  }
  return wrapped;
}

process.nextTick = wrapWithActivateUvLoop(process.nextTick);

global.setImmediate = timers.setImmediate = wrapWithActivateUvLoop(timers.setImmediate);
global.clearImmediate = timers.clearImmediate;

// 当出现以下情况时，setTimeout需要更新事件循环的轮询超时。
// 在Chromium的事件循环下调用，节点的事件循环将不会有机会。
// 来更新超时，所以我们必须强制节点的事件循环。
// 重新计算浏览器进程中的超时时间。
timers.setTimeout = wrapWithActivateUvLoop(timers.setTimeout);
timers.setInterval = wrapWithActivateUvLoop(timers.setInterval);

// 仅覆盖浏览器进程中的全局setTimeout/setInterval隐含。
if (process.type === 'browser') {
  global.setTimeout = timers.setTimeout;
  global.setInterval = timers.setInterval;
}

if (process.platform === 'win32') {
  // 始终为标准输入流返回EOF。
  const { Readable } = require('stream');
  const stdin = new Readable();
  stdin.push(null);
  Object.defineProperty(process, 'stdin', {
    configurable: false,
    enumerable: true,
    get () {
      return stdin;
    }
  });
}
