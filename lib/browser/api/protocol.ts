import { app, session } from 'electron/main';

// 全局协议API。
const protocol = process._linkedBinding('electron_browser_protocol');

// 默认会话的回退协议API。
Object.setPrototypeOf(protocol, new Proxy({}, {
  get (_target, property) {
    if (!app.isReady()) return;

    const protocol = session.defaultSession!.protocol;
    if (!Object.prototype.hasOwnProperty.call(protocol, property)) return;

    // 直接返回本机函数会引发错误。
    return (...args: any[]) => (protocol[property as keyof Electron.Protocol] as Function)(...args);
  },

  ownKeys () {
    if (!app.isReady()) return [];
    return Reflect.ownKeys(session.defaultSession!.protocol);
  },

  has: (target, property: string) => {
    if (!app.isReady()) return false;
    return Reflect.has(session.defaultSession!.protocol, property);
  },

  getOwnPropertyDescriptor () {
    return { configurable: true, enumerable: true };
  }
}));

export default protocol;
