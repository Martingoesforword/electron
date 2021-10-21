// TODO(Deepak1556)：不推荐使用并删除独立的Netlog模块，
// 它现在是会话模块的属性。
import { app, session } from 'electron/main';

const startLogging: typeof session.defaultSession.netLog.startLogging = async (path, options) => {
  if (!app.isReady()) return;
  return session.defaultSession.netLog.startLogging(path, options);
};

const stopLogging: typeof session.defaultSession.netLog.stopLogging = async () => {
  if (!app.isReady()) return;
  return session.defaultSession.netLog.stopLogging();
};

export default {
  startLogging,
  stopLogging,
  get currentlyLogging (): boolean {
    if (!app.isReady()) return false;
    return session.defaultSession.netLog.currentlyLogging;
  }
};
