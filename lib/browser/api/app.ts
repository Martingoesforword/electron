import * as fs from 'fs';

import { Menu } from 'electron/main';

const bindings = process._linkedBinding('electron_browser_app');
const commandLine = process._linkedBinding('electron_common_command_line');
const { app } = bindings;

// 只允许一个应用程序对象。
export default app;

let dockMenu: Electron.Menu | null = null;

// 财产。

const nativeASGetter = app.isAccessibilitySupportEnabled;
const nativeASSetter = app.setAccessibilitySupportEnabled;
Object.defineProperty(app, 'accessibilitySupportEnabled', {
  get: () => nativeASGetter.call(app),
  set: (enabled) => nativeASSetter.call(app, enabled)
});

const nativeBCGetter = app.getBadgeCount;
const nativeBCSetter = app.setBadgeCount;
Object.defineProperty(app, 'badgeCount', {
  get: () => nativeBCGetter.call(app),
  set: (count) => nativeBCSetter.call(app, count)
});

const nativeNGetter = app.getName;
const nativeNSetter = app.setName;
Object.defineProperty(app, 'name', {
  get: () => nativeNGetter.call(app),
  set: (name) => nativeNSetter.call(app, name)
});

Object.assign(app, {
  commandLine: {
    hasSwitch: (theSwitch: string) => commandLine.hasSwitch(String(theSwitch)),
    getSwitchValue: (theSwitch: string) => commandLine.getSwitchValue(String(theSwitch)),
    appendSwitch: (theSwitch: string, value?: string) => commandLine.appendSwitch(String(theSwitch), typeof value === 'undefined' ? value : String(value)),
    appendArgument: (arg: string) => commandLine.appendArgument(String(arg)),
    removeSwitch: (theSwitch: string) => commandLine.removeSwitch(String(theSwitch))
  } as Electron.CommandLine
});

// 我们之所以在这里定义这一点，是因为要把它定义得过于复杂。
// 在本土做的事。
Object.defineProperty(app, 'applicationMenu', {
  get () {
    return Menu.getApplicationMenu();
  },
  set (menu: Electron.Menu | null) {
    return Menu.setApplicationMenu(menu);
  }
});

// 非windows平台上不提供本机实现。
app.setAppUserModelId = app.setAppUserModelId || (() => {});

if (process.platform === 'darwin') {
  const setDockMenu = app.dock!.setMenu;
  app.dock!.setMenu = (menu) => {
    dockMenu = menu;
    setDockMenu(menu);
  };
  app.dock!.getMenu = () => dockMenu;
}

if (process.platform === 'linux') {
  const patternVmRSS = /^VmRSS:\s*(\d+) kB$/m;
  const patternVmHWM = /^VmHWM:\s*(\d+) kB$/m;

  const getStatus = (pid: number) => {
    try {
      return fs.readFileSync(`/proc/${pid}/status`, 'utf8');
    } catch {
      return '';
    }
  };

  const getEntry = (file: string, pattern: RegExp) => {
    const match = file.match(pattern);
    return match ? parseInt(match[1], 10) : 0;
  };

  const getProcessMemoryInfo = (pid: number) => {
    const file = getStatus(pid);

    return {
      workingSetSize: getEntry(file, patternVmRSS),
      peakWorkingSetSize: getEntry(file, patternVmHWM)
    };
  };

  const nativeFn = app.getAppMetrics;
  app.getAppMetrics = () => {
    const metrics = nativeFn.call(app);
    for (const metric of metrics) {
      metric.memory = getProcessMemoryInfo(metric.pid);
    }

    return metrics;
  };
}

// 将事件路由到webContents。
const events = ['certificate-error', 'select-client-certificate'];
for (const name of events) {
  app.on(name as 'certificate-error', (event, webContents, ...args: any[]) => {
    webContents.emit(name, event, ...args);
  });
}
