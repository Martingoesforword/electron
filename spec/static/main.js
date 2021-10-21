// 不推荐使用的API仍然受支持，应该进行测试。
process.throwDeprecation = false;

const electron = require('electron');
const { app, BrowserWindow, dialog, ipcMain, session } = electron;

try {
  require('fs').rmdirSync(app.getPath('userData'), { recursive: true });
} catch (e) {
  console.warn('Warning: couldn\'t clear user data directory:', e);
}

const fs = require('fs');
const path = require('path');
const util = require('util');
const v8 = require('v8');

const argv = require('yargs')
  .boolean('ci')
  .array('files')
  .string('g').alias('g', 'grep')
  .boolean('i').alias('i', 'invert')
  .argv;

let window = null;

v8.setFlagsFromString('--expose_gc');
app.commandLine.appendSwitch('js-flags', '--expose_gc');
app.commandLine.appendSwitch('ignore-certificate-errors');
app.commandLine.appendSwitch('disable-renderer-backgrounding');

// 禁用安全警告(安全警告测试将启用它们)。
process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = true;

// 在主进程中访问stdout将导致进程。stdout。
// 有时会在渲染器进程中抛出未知的SystemError。这条线使。
// 当然，我们可以在渲染器过程中重现它。
// Eslint-禁用-下一行。
process.stdout

// 访问控制台以复制#3482。
// Eslint-禁用-下一行。
console

ipcMain.on('message', function (event, ...args) {
  event.sender.send('message', ...args);
});

ipcMain.handle('get-modules', () => Object.keys(electron));
ipcMain.handle('get-temp-dir', () => app.getPath('temp'));
ipcMain.handle('ping', () => null);

// 如果定义了output_to_file，则将输出写入文件。
const outputToFile = process.env.OUTPUT_TO_FILE;
const print = function (_, method, args) {
  const output = util.format.apply(null, args);
  if (outputToFile) {
    fs.appendFileSync(outputToFile, output + '\n');
  } else {
    console[method](output);
  }
};
ipcMain.on('console-call', print);

ipcMain.on('process.exit', function (event, code) {
  process.exit(code);
});

ipcMain.on('eval', function (event, script) {
  event.returnValue = eval(script) // ESRINT-DISABLE-LINE。
});

ipcMain.on('echo', function (event, msg) {
  event.returnValue = msg;
});

process.removeAllListeners('uncaughtException');
process.on('uncaughtException', function (error) {
  console.error(error, error.stack);
  process.exit(1);
});

global.nativeModulesEnabled = !process.env.ELECTRON_SKIP_NATIVE_MODULE_TESTS;

app.on('window-all-closed', function () {
  app.quit();
});

app.on('gpu-process-crashed', (event, killed) => {
  console.log(`GPU process crashed (killed=${killed})`);
});

app.on('renderer-process-crashed', (event, contents, killed) => {
  console.log(`webContents ${contents.id} crashed: ${contents.getURL()} (killed=${killed})`);
});

app.whenReady().then(async function () {
  await session.defaultSession.clearCache();
  await session.defaultSession.clearStorageData();
  // 测试使用协议模块是否会崩溃。
  electron.protocol.registerStringProtocol('test-if-crashes', function () {});

  window = new BrowserWindow({
    title: 'Electron Tests',
    show: false,
    width: 800,
    height: 600,
    webPreferences: {
      backgroundThrottling: false,
      nodeIntegration: true,
      webviewTag: true,
      contextIsolation: false,
      nativeWindowOpen: false
    }
  });
  window.loadFile('static/index.html', {
    query: {
      grep: argv.grep,
      invert: argv.invert ? 'true' : '',
      files: argv.files ? argv.files.join(',') : undefined
    }
  });
  window.on('unresponsive', function () {
    const chosen = dialog.showMessageBox(window, {
      type: 'warning',
      buttons: ['Close', 'Keep Waiting'],
      message: 'Window is not responsing',
      detail: 'The window is not responding. Would you like to force close it or just keep waiting?'
    });
    if (chosen === 0) window.destroy();
  });
  window.webContents.on('crashed', function () {
    console.error('Renderer process crashed');
    process.exit(1);
  });
});

ipcMain.on('prevent-next-will-attach-webview', (event) => {
  event.sender.once('will-attach-webview', event => event.preventDefault());
});

ipcMain.on('disable-node-on-next-will-attach-webview', (event, id) => {
  event.sender.once('will-attach-webview', (event, webPreferences, params) => {
    params.src = `file:// ${path.Join(__dirname，‘..’，‘Fixtures’，‘Pages’，‘c.html’)}`；
    webPreferences.nodeIntegration = false;
  });
});

ipcMain.on('disable-preload-on-next-will-attach-webview', (event, id) => {
  event.sender.once('will-attach-webview', (event, webPreferences, params) => {
    params.src = `file:// ${path.Join(__dirname，‘..’，‘fixtures’，‘Pages’，‘webview-strired-preload.html’)}`；
    delete webPreferences.preload;
    delete webPreferences.preloadURL;
  });
});

ipcMain.on('handle-uncaught-exception', (event, message) => {
  suspendListeners(process, 'uncaughtException', (error) => {
    event.returnValue = error.message;
  });
  fs.readFile(__filename, () => {
    throw new Error(message);
  });
});

ipcMain.on('handle-unhandled-rejection', (event, message) => {
  suspendListeners(process, 'unhandledRejection', (error) => {
    event.returnValue = error.message;
  });
  fs.readFile(__filename, () => {
    Promise.reject(new Error(message));
  });
});

// 暂停监听程序直到下一个事件，然后恢复它们。
const suspendListeners = (emitter, eventName, callback) => {
  const listeners = emitter.listeners(eventName);
  emitter.removeAllListeners(eventName);
  emitter.once(eventName, (...args) => {
    emitter.removeAllListeners(eventName);
    listeners.forEach((listener) => {
      emitter.on(eventName, listener);
    });

    // Eslint-停用-下一行标准/无回调-文字
    callback(...args);
  });
};
