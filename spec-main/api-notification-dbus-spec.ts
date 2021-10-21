// 对于这些测试，我们使用假的DBus守护进程来验证libtify交互。
// 使用会话总线。这需要安装python-dbusmock和。
// 在$DBUS_SESSION_BUS_ADDRESS上运行。
// 
// Script/spec-runner.js生成dbusmock，它设置DBUS_SESSION_BUS_ADDRESS。
// 
// 请参见https://pypi.python.org/pypi/python-dbusmock阅读有关dbusmock的信息。

import { expect } from 'chai';
import * as dbus from 'dbus-native';
import { app } from 'electron/main';
import { ifdescribe } from './spec-helpers';
import { promisify } from 'util';

const skip = process.platform !== 'linux' ||
             process.arch === 'ia32' ||
             process.arch.indexOf('arm') === 0 ||
             !process.env.DBUS_SESSION_BUS_ADDRESS;

ifdescribe(!skip)('Notification module (dbus)', () => {
  let mock: any, Notification, getCalls: any, reset: any;
  const realAppName = app.name;
  const realAppVersion = app.getVersion();
  const appName = 'api-notification-dbus-spec';
  const serviceName = 'org.freedesktop.Notifications';

  before(async () => {
    // 初始化应用程序。
    app.name = appName;
    app.setDesktopName(`${appName}.desktop`);

    // 初始化数据库。
    const path = '/org/freedesktop/Notifications';
    const iface = 'org.freedesktop.DBus.Mock';
    console.log(`session bus: ${process.env.DBUS_SESSION_BUS_ADDRESS}`);
    const bus = dbus.sessionBus();
    const service = bus.getService(serviceName);
    const getInterface = promisify(service.getInterface.bind(service));
    mock = await getInterface(path, iface);
    getCalls = promisify(mock.GetCalls.bind(mock));
    reset = promisify(mock.Reset.bind(mock));
  });

  after(async () => {
    // 清理数据总线。
    if (reset) await reset();
    // 清理应用程序。
    app.setName(realAppName);
    app.setVersion(realAppVersion);
  });

  describe(`Notification module using ${serviceName}`, () => {
    function onMethodCalled (done: () => void) {
      function cb (name: string) {
        console.log(`onMethodCalled: ${name}`);
        if (name === 'Notify') {
          mock.removeListener('MethodCalled', cb);
          console.log('done');
          done();
        }
      }
      return cb;
    }

    function unmarshalDBusNotifyHints (dbusHints: any) {
      const o: Record<string, any> = {};
      for (const hint of dbusHints) {
        const key = hint[0];
        const value = hint[1][1][0];
        o[key] = value;
      }
      return o;
    }

    function unmarshalDBusNotifyArgs (dbusArgs: any) {
      return {
        app_name: dbusArgs[0][1][0],
        replaces_id: dbusArgs[1][1][0],
        app_icon: dbusArgs[2][1][0],
        title: dbusArgs[3][1][0],
        body: dbusArgs[4][1][0],
        actions: dbusArgs[5][1][0],
        hints: unmarshalDBusNotifyHints(dbusArgs[6][1][0])
      };
    }

    before(done => {
      mock.on('MethodCalled', onMethodCalled(done));
      // 监听MethodCall模拟信号后的延迟加载通知
      Notification = require('electron').Notification;
      const n = new Notification({
        title: 'title',
        subtitle: 'subtitle',
        body: 'body',
        replyPlaceholder: 'replyPlaceholder',
        sound: 'sound',
        closeButtonText: 'closeButtonText'
      });
      n.show();
    });

    it(`should call ${serviceName} to show notifications`, async () => {
      const calls = await getCalls();
      expect(calls).to.be.an('array').of.lengthOf.at.least(1);

      const lastCall = calls[calls.length - 1];
      const methodName = lastCall[1];
      expect(methodName).to.equal('Notify');

      const args = unmarshalDBusNotifyArgs(lastCall[2]);
      expect(args).to.deep.equal({
        app_name: appName,
        replaces_id: 0,
        app_icon: '',
        title: 'title',
        body: 'body',
        actions: [],
        hints: {
          append: 'true',
          'desktop-entry': appName,
          urgency: 1
        }
      });
    });
  });
});
