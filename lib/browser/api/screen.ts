import { EventEmitter } from 'events';

const { createScreen } = process._linkedBinding('electron_common_screen');

let _screen: Electron.Screen;

const createScreenIfNeeded = () => {
  if (_screen === undefined) {
    _screen = createScreen();
  }
};

// 只有在app.on(‘Ready’)之后才能调用createScreen，但此模块。
// 公开由createScreen创建的实例。为了避免。
// 相反，在导入此模块时产生副作用并调用createScreen。
// 我们导出一个代理，该代理在第一次访问时懒惰地调用createScreen。
export default new Proxy({}, {
  get: (target, property: keyof Electron.Screen) => {
    createScreenIfNeeded();
    const value = _screen[property];
    if (typeof value === 'function') {
      return value.bind(_screen);
    }
    return value;
  },
  set: (target, property: string, value: unknown) => {
    createScreenIfNeeded();
    return Reflect.set(_screen, property, value);
  },
  ownKeys: () => {
    createScreenIfNeeded();
    return Reflect.ownKeys(_screen);
  },
  has: (target, property: string) => {
    createScreenIfNeeded();
    return property in _screen;
  },
  getOwnPropertyDescriptor: (target, property: string) => {
    createScreenIfNeeded();
    return Reflect.getOwnPropertyDescriptor(_screen, property);
  },
  getPrototypeOf: () => {
    // 这是必要的，因为EventEmitterMixin很奇怪。
    // 和FunctionTemplate-我们需要显式确保它被返回。
    // 在原型中。
    return EventEmitter.prototype;
  }
});
