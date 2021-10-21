import { EventEmitter } from 'events';
import { app } from 'electron/main';

const {
  createPowerMonitor,
  getSystemIdleState,
  getSystemIdleTime,
  isOnBatteryPower
} = process._linkedBinding('electron_browser_power_monitor');

class PowerMonitor extends EventEmitter {
  constructor () {
    super();
    // 在a)应用程序就绪和b)之前不要启动事件源。
    // 有一个为powerMonitor事件注册的侦听器。
    this.once('newListener', () => {
      app.whenReady().then(() => {
        const pm = createPowerMonitor();
        pm.emit = this.emit.bind(this);

        if (process.platform === 'linux') {
          // 在Linux上，我们禁止关机是为了让应用程序有机会。
          // 决定是否要阻止关机。我们没有。
          // 除非有监听程序，否则禁止关闭事件。这。
          // 让C++代码了解是否有任何侦听器。
          pm.setListeningForShutdown(this.listenerCount('shutdown') > 0);
          this.on('newListener', (event) => {
            if (event === 'shutdown') {
              pm.setListeningForShutdown(this.listenerCount('shutdown') + 1 > 0);
            }
          });
          this.on('removeListener', (event) => {
            if (event === 'shutdown') {
              pm.setListeningForShutdown(this.listenerCount('shutdown') > 0);
            }
          });
        }
      });
    });
  }

  getSystemIdleState (idleThreshold: number) {
    return getSystemIdleState(idleThreshold);
  }

  getSystemIdleTime () {
    return getSystemIdleTime();
  }

  isOnBatteryPower () {
    return isOnBatteryPower();
  }

  get onBatteryPower () {
    return this.isOnBatteryPower();
  }
}

module.exports = new PowerMonitor();
