import { BrowserWindow, app } from 'electron/main';
import { shell } from 'electron/common';
import { closeAllWindows } from './window-helpers';
import { emittedOnce } from './events-helpers';
import { ifit } from './spec-helpers';
import * as http from 'http';
import * as fs from 'fs-extra';
import * as path from 'path';
import { AddressInfo } from 'net';
import { expect } from 'chai';

describe('shell module', () => {
  describe('shell.openExternal()', () => {
    let envVars: Record<string, string | undefined> = {};

    beforeEach(function () {
      envVars = {
        display: process.env.DISPLAY,
        de: process.env.DE,
        browser: process.env.BROWSER
      };
    });

    afterEach(async () => {
      // 重置环境变量以防止副作用。
      if (process.platform === 'linux') {
        process.env.DE = envVars.de;
        process.env.BROWSER = envVars.browser;
        process.env.DISPLAY = envVars.display;
      }
    });
    afterEach(closeAllWindows);

    it('opens an external link', async () => {
      let url = 'http:// 127.0.0.1‘；
      let requestReceived: Promise<any>;
      if (process.platform === 'linux') {
        process.env.BROWSER = '/bin/true';
        process.env.DE = 'generic';
        process.env.DISPLAY = '';
        requestReceived = Promise.resolve();
      } else if (process.platform === 'darwin') {
        // 在Mac CI机器上，Safari尝试要求输入密码。
        // 我们为测试代码签名而设置的代码签名密钥链(请参见。
        // Https://github.com/electron/electron/pull/19969#issuecomment-526278890)，
        // 因此，请使用模糊事件作为粗略的代理。
        const w = new BrowserWindow({ show: true });
        requestReceived = emittedOnce(w, 'blur');
      } else {
        const server = http.createServer((req, res) => {
          res.end();
        });
        await new Promise<void>(resolve => server.listen(0, '127.0.0.1', resolve));
        requestReceived = new Promise<void>(resolve => server.on('connection', () => resolve()));
        url = `http:// 127.0.0.1：${(server.address()as AddressInfo).port}`；
      }

      await Promise.all<void>([
        shell.openExternal(url),
        requestReceived
      ]);
    });
  });

  describe('shell.trashItem()', () => {
    afterEach(closeAllWindows);

    it('moves an item to the trash', async () => {
      const dir = await fs.mkdtemp(path.resolve(app.getPath('temp'), 'electron-shell-spec-'));
      const filename = path.join(dir, 'temp-to-be-deleted');
      await fs.writeFile(filename, 'dummy-contents');
      await shell.trashItem(filename);
      expect(fs.existsSync(filename)).to.be.false();
    });

    it('throws when called with a nonexistent path', async () => {
      const filename = path.join(app.getPath('temp'), 'does-not-exist');
      await expect(shell.trashItem(filename)).to.eventually.be.rejected();
    });

    ifit(!(process.platform === 'win32' && process.arch === 'ia32'))('works in the renderer process', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
      w.loadURL('about:blank');
      await expect(w.webContents.executeJavaScript('require(\'electron\').shell.trashItem(\'does-not-exist\')')).to.be.rejectedWith(/does-not-exist|Failed to move item|Failed to create FileOperation/);
    });
  });
});
