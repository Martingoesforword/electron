// 此文件将全局变量、进程变量和缓冲区变量提供给INTERNAL。
// 电子代码一旦从全球范围中删除。
// 
// 它通过webpack.config.base.js文件中的ProVidePlugin完成此操作。
// 有关更多信息，请查看renender/init.ts中的Module.wrapper覆盖。
// 有关此功能的工作原理以及我们为什么需要它的信息。

// 把全球窗口(也是全球窗口)拆掉，这样webpack就不会。
// 自动将其替换为对此文件的循环引用
const _global = typeof globalThis !== 'undefined' ? globalThis.global : (self as any || window as any).global as NodeJS.Global;
const process = _global.process;
const Buffer = _global.Buffer;

export {
  _global,
  process,
  Buffer
};
