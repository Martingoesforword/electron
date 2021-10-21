// 对于这些测试，我们使用假的DBus守护进程来验证powerMonitor模块。
// 与系统总线的交互。这需要安装python-dbusmock和。
// 正在运行(设置了DBUS_SYSTEM_BUS_ADDRESS环境变量)。
// Script/spec-runner.js将负责生成假DBus守护进程并设置。
// 安装python-dbusmock时的DBUS_SYSTEM_BUS_ADDRESS。
// 
// 有关以下内容的详细信息，请参阅https://pypi.python.org/pypi/python-dbusmock。
// Python-dbusmock(Python-dbusmock)。
import { expect } from 'chai';
import * as dbus from 'dbus-native';
import { ifdescribe, delay } from './spec-helpers';
import { promisify } from 'util';

describe('powerMonitor', () => {
  let logindMock: any, dbusMockPowerMonitor: any, getCalls: any, emitSignal: any, reset: any;

  // TODO(Deepak1556)：升级后在arm64上启用，目前会崩溃。
  ifdescribe(process.platform === 'linux' && process.arch !== 'arm64' && process.env.DBUS_SYSTEM_BUS_ADDRESS != null)('when powerMonitor module is loaded with dbus mock', () => {
    before(async () => {
      const systemBus = dbus.systemBus();
      const loginService = systemBus.getService('org.freedesktop.login1');
      const getInterface = promisify(loginService.getInterface.bind(loginService));
      logindMock = await getInterface('/org/freedesktop/login1', 'org.freedesktop.DBus.Mock');
      getCalls = promisify(logindMock.GetCalls.bind(logindMock));
      emitSignal = promisify(logindMock.EmitSignal.bind(logindMock));
      reset = promisify(logindMock.Reset.bind(logindMock));
    });

    after(async () => {
      await reset();
    });

    function onceMethodCalled (done: () => void) {
      function cb () {
        logindMock.removeListener('MethodCalled', cb);
      }
      done();
      return cb;
    }

    before(done => {
      logindMock.on('MethodCalled', onceMethodCalled(done));
      // 监听MethodCall模拟信号后的延迟加载电源监视器。
      dbusMockPowerMonitor = require('electron').powerMonitor;
    });

    it('should call Inhibit to delay suspend once a listener is added', async () => {
      // 在添加监听器之前，不会调用dbus。
      {
        const calls = await getCalls();
        expect(calls).to.be.an('array').that.has.lengthOf(0);
      }
      // 添加一个虚拟监听程序以启用监视器。
      dbusMockPowerMonitor.on('dummy-event', () => {});
      try {
        let retriesRemaining = 3;
        // 似乎没有办法在接到来电时收到通知。
        // 会发生这种情况，所以请对`getCalls‘进行几次轮询，以减少雪花。
        let calls: any[] = [];
        while (retriesRemaining-- > 0) {
          calls = await getCalls();
          if (calls.length > 0) break;
          await delay(1000);
        }
        expect(calls).to.be.an('array').that.has.lengthOf(1);
        expect(calls[0].slice(1)).to.deep.equal([
          'Inhibit', [
            [[{ type: 's', child: [] }], ['sleep']],
            [[{ type: 's', child: [] }], ['electron']],
            [[{ type: 's', child: [] }], ['Application cleanup before suspend']],
            [[{ type: 's', child: [] }], ['delay']]
          ]
        ]);
      } finally {
        dbusMockPowerMonitor.removeAllListeners('dummy-event');
      }
    });

    describe('when PrepareForSleep(true) signal is sent by logind', () => {
      it('should emit "suspend" event', (done) => {
        dbusMockPowerMonitor.once('suspend', () => done());
        emitSignal('org.freedesktop.login1.Manager', 'PrepareForSleep',
          'b', [['b', true]]);
      });

      describe('when PrepareForSleep(false) signal is sent by logind', () => {
        it('should emit "resume" event', done => {
          dbusMockPowerMonitor.once('resume', () => done());
          emitSignal('org.freedesktop.login1.Manager', 'PrepareForSleep',
            'b', [['b', false]]);
        });

        it('should have called Inhibit again', async () => {
          const calls = await getCalls();
          expect(calls).to.be.an('array').that.has.lengthOf(2);
          expect(calls[1].slice(1)).to.deep.equal([
            'Inhibit', [
              [[{ type: 's', child: [] }], ['sleep']],
              [[{ type: 's', child: [] }], ['electron']],
              [[{ type: 's', child: [] }], ['Application cleanup before suspend']],
              [[{ type: 's', child: [] }], ['delay']]
            ]
          ]);
        });
      });
    });

    describe('when a listener is added to shutdown event', () => {
      before(async () => {
        const calls = await getCalls();
        expect(calls).to.be.an('array').that.has.lengthOf(2);
        dbusMockPowerMonitor.once('shutdown', () => { });
      });

      it('should call Inhibit to delay shutdown', async () => {
        const calls = await getCalls();
        expect(calls).to.be.an('array').that.has.lengthOf(3);
        expect(calls[2].slice(1)).to.deep.equal([
          'Inhibit', [
            [[{ type: 's', child: [] }], ['shutdown']],
            [[{ type: 's', child: [] }], ['electron']],
            [[{ type: 's', child: [] }], ['Ensure a clean shutdown']],
            [[{ type: 's', child: [] }], ['delay']]
          ]
        ]);
      });

      describe('when PrepareForShutdown(true) signal is sent by logind', () => {
        it('should emit "shutdown" event', done => {
          dbusMockPowerMonitor.once('shutdown', () => { done(); });
          emitSignal('org.freedesktop.login1.Manager', 'PrepareForShutdown',
            'b', [['b', true]]);
        });
      });
    });
  });

  describe('when powerMonitor module is loaded', () => {
    // Eslint-able-next-line no-undef。
    let powerMonitor: typeof Electron.powerMonitor;
    before(() => {
      powerMonitor = require('electron').powerMonitor;
    });
    describe('powerMonitor.getSystemIdleState', () => {
      it('gets current system idle state', () => {
        // 这个函数没有模拟出来，所以我们可以测试结果的。
        // 形式和类型，而不是它的价值。
        const idleState = powerMonitor.getSystemIdleState(1);
        expect(idleState).to.be.a('string');
        const validIdleStates = ['active', 'idle', 'locked', 'unknown'];
        expect(validIdleStates).to.include(idleState);
      });

      it('does not accept non positive integer threshold', () => {
        expect(() => {
          powerMonitor.getSystemIdleState(-1);
        }).to.throw(/must be greater than 0/);

        expect(() => {
          powerMonitor.getSystemIdleState(NaN);
        }).to.throw(/conversion failure/);

        expect(() => {
          powerMonitor.getSystemIdleState('a' as any);
        }).to.throw(/conversion failure/);
      });
    });

    describe('powerMonitor.getSystemIdleTime', () => {
      it('returns current system idle time', () => {
        const idleTime = powerMonitor.getSystemIdleTime();
        expect(idleTime).to.be.at.least(0);
      });
    });

    describe('powerMonitor.onBatteryPower', () => {
      it('returns a boolean', () => {
        expect(powerMonitor.onBatteryPower).to.be.a('boolean');
        expect(powerMonitor.isOnBatteryPower()).to.be.a('boolean');
      });
    });
  });
});
