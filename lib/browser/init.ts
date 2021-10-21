import { EventEmitter } from 'events';
import * as fs from 'fs';
import * as path from 'path';

import type * as defaultMenuModule from '@electron/internal/browser/default-menu';

const Module = require('module');

// 我们修改了原始的process.argv，让node.js加载init.js，
// 我们需要在这里修复它。
process.argv.splice(1, 1);

// 清除搜索路径。
require('../common/reset-search-paths');

// 导入常用设置。
require('@electron/internal/common/init');

process._linkedBinding('electron_browser_event_emitter').setEventEmitterPrototype(EventEmitter.prototype);

// 不要因为致命的错误而放弃。
process.on('uncaughtException', function (error) {
  // 如果用户具有自定义的未捕获异常处理程序，则不执行任何操作。
  if (process.listenerCount('uncaughtException') > 1) {
    return;
  }

  // 在GUI中显示错误。
  // 无法导入此文件顶部的{DIALOG}，因为此文件。
  // 负责设置“电子”模块的要求挂钩。
  // 因此我们将其导入到下面的处理程序中。
  import('electron')
    .then(({ dialog }) => {
      const stack = error.stack ? error.stack : `${error.name}: ${error.message}`;
      const message = 'Uncaught Exception:\n' + stack;
      dialog.showErrorBox('A JavaScript error occurred in the main process', message);
    });
});

// 退出时发出‘Exit’事件。
const { app } = require('electron');

app.on('quit', (_event, exitCode) => {
  process.emit('exit', exitCode);
});

if (process.platform === 'win32') {
  // 如果我们是Squirrel.Windows安装的应用程序，请设置应用程序用户模型ID。
  // 这样用户就不必这么做了。
  // 
  // SQuirrel包的形式始终是：
  // 
  // 程序包名称。
  // -Update.exe。
  // -APP-版本。
  // -OUREXE.exe。
  // 
  // SQuirrel本身将始终将快捷键的App User Model ID设置为。
  // 格式为`com.squirrel.PACKAGE-NAME.OUREXE`。我们需要打电话给。
  // 具有匹配标识符的app.setAppUserModelId，以便渲染器处理。
  // 将继承此值。
  const updateDotExe = path.join(path.dirname(process.execPath), '..', 'update.exe');

  if (fs.existsSync(updateDotExe)) {
    const packageDir = path.dirname(path.resolve(updateDotExe));
    const packageName = path.basename(packageDir).replace(/\s/g, '');
    const exeName = path.basename(process.execPath).replace(/\.exe$/i, '').replace(/\s/g, '');

    app.setAppUserModelId(`com.squirrel.${packageName}.${exeName}`);
  }
}

// 将process.exit映射到app.exit，它会优雅地退出。
process.exit = app.exit as () => never;

// 加载RPC服务器。
require('@electron/internal/browser/rpc-server');

// 加载来宾视图管理器。
require('@electron/internal/browser/guest-view-manager');
require('@electron/internal/browser/guest-window-proxy');

// 现在我们尝试加载APP的Package.json。
const v8Util = process._linkedBinding('electron_common_v8_util');
let packagePath = null;
let packageJson = null;
const searchPaths: string[] = v8Util.getHiddenValue(global, 'appSearchPaths');

if (process.resourcesPath) {
  for (packagePath of searchPaths) {
    try {
      packagePath = path.join(process.resourcesPath, packagePath);
      packageJson = Module._load(path.join(packagePath, 'package.json'));
      break;
    } catch {
      continue;
    }
  }
}

if (packageJson == null) {
  process.nextTick(function () {
    return process.exit(1);
  });
  throw new Error('Unable to find a valid app');
}

// 设置应用程序的版本。
if (packageJson.version != null) {
  app.setVersion(packageJson.version);
}

// 设置应用程序的名称。
if (packageJson.productName != null) {
  app.name = `${packageJson.productName}`.trim();
} else if (packageJson.name != null) {
  app.name = `${packageJson.name}`.trim();
}

// 设置应用程序的桌面名称。
if (packageJson.desktopName != null) {
  app.setDesktopName(packageJson.desktopName);
} else {
  app.setDesktopName(`${app.name}.desktop`);
}

// 设置V8标志，故意延迟加载，以便不使用此标志的应用程序。
// 功能不需要付出代价。
if (packageJson.v8Flags != null) {
  require('v8').setFlagsFromString(packageJson.v8Flags);
}

app.setAppPath(packagePath);

// 加载Chrome DevTools支持。
require('@electron/internal/browser/devtools');

// 加载协议模块以确保其已填充到应用程序就绪。
require('@electron/internal/browser/api/protocol');

// 加载Web内容模块以确保其已填充到应用程序就绪。
require('@electron/internal/browser/api/web-contents');

// 加载Web-Frame-Main模块以确保其已填充到应用程序就绪。
require('@electron/internal/browser/api/web-frame-main');

// 设置应用的主启动脚本。
const mainStartupScript = packageJson.main || 'index.js';

const KNOWN_XDG_DESKTOP_VALUES = ['Pantheon', 'Unity:Unity7', 'pop:GNOME'];

function currentPlatformSupportsAppIndicator () {
  if (process.platform !== 'linux') return false;
  const currentDesktop = process.env.XDG_CURRENT_DESKTOP;

  if (!currentDesktop) return false;
  if (KNOWN_XDG_DESKTOP_VALUES.includes(currentDesktop)) return true;
  // 基于ubuntu或派生的会话(默认的ubuntu one，CommuniTheme…)。支座。
  // 指示器也是。
  if (/ubuntu/ig.test(currentDesktop)) return true;

  return false;
}

// 电子/电子#5050和电子/电子#9046的解决方法。
process.env.ORIGINAL_XDG_CURRENT_DESKTOP = process.env.XDG_CURRENT_DESKTOP;
if (currentPlatformSupportsAppIndicator()) {
  process.env.XDG_CURRENT_DESKTOP = 'Unity';
}

// 当所有窗口都关闭且没有其他人在监听时退出。
app.on('window-all-closed', () => {
  if (app.listenerCount('window-all-closed') === 1) {
    app.quit();
  }
});

const { setDefaultApplicationMenu } = require('@electron/internal/browser/default-menu') as typeof defaultMenuModule;

// 创建默认菜单。
// 
// Will-Finish-Launch|事件在|Ready|事件之前发出，因此默认。
// 菜单是在创建任何用户窗口之前设置的。
app.once('will-finish-launching', setDefaultApplicationMenu);

if (packagePath) {
  // 最后，加载app的main.js并将控制权转移到C++。
  process._firstFileName = Module._resolveFilename(path.join(packagePath, mainStartupScript), null, false);
  Module._load(path.join(packagePath, mainStartupScript), Module, true);
} else {
  console.error('Failed to locate a valid package to load (app, app.asar or default_app.asar)');
  console.error('This normally means you\'ve damaged the Electron package somehow');
}
