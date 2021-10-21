import * as path from 'path';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import type * as ipcRendererInternalModule from '@electron/internal/renderer/ipc-renderer-internal';
import type * as webFrameInitModule from '@electron/internal/renderer/web-frame-init';
import type * as webViewInitModule from '@electron/internal/renderer/web-view/web-view-init';
import type * as windowSetupModule from '@electron/internal/renderer/window-setup';
import type * as securityWarningsModule from '@electron/internal/renderer/security-warnings';

const Module = require('module');

// 确保像“process”和“global”这样的全局变量在预加载中始终可用。
// 脚本即使在“已加载”脚本中被删除后也是如此。
// 
// 注1：我们依赖节点补丁来实际传递“进程”和“全局”以及。
// 包装器的其他参数。
// 
// 注2：Node引入了一个新的代码路径来使用本机代码包装模块。
// 代码，它不能与这次黑客攻击一起工作。但是，通过修改。
// “Module.wrapper”我们可以强制Node使用旧的代码路径来包装模块。
// 使用JavaScript编写代码。
// 
// 注3：我们在内部通过。
// Webpack在webpack.config.base.js中提供了视频插件。如果你加了任何额外的。
// 变量添加到此包装器，请确保同时更新该插件。
Module.wrapper = [
  '(function (exports, require, module, __filename, __dirname, process, global, Buffer) { ' +
  // 通过在新的闭包中运行代码，模块可以。
  // 用局部变量覆盖“process”和“buffer”的代码。
  'return function (exports, require, module, __filename, __dirname) { ',
  '\n}.call(this, exports, require, module, __filename, __dirname); });'
];

// 我们修改了原始的process.argv，让node.js加载。
// Init.js，我们需要在这里恢复它。
process.argv.splice(1, 1);

// 清除搜索路径。

require('../common/reset-search-paths');

// 导入常用设置。
require('@electron/internal/common/init');

// IPC将使用全局变量进行事件调度。
const v8Util = process._linkedBinding('electron_common_v8_util');

const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal') as typeof ipcRendererInternalModule;
const ipcRenderer = require('@electron/internal/renderer/api/ipc-renderer').default;

v8Util.setHiddenValue(global, 'ipcNative', {
  onMessage (internal: boolean, channel: string, ports: any[], args: any[], senderId: number) {
    if (internal && senderId !== 0) {
      console.error(`Message ${channel} sent by unexpected WebContents (${senderId})`);
      return;
    }
    const sender = internal ? ipcRendererInternal : ipcRenderer;
    sender.emit(channel, { sender, senderId, ports }, ...args);
  }
});

process.getProcessMemoryInfo = () => {
  return ipcRendererInternal.invoke<Electron.ProcessMemoryInfo>(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO);
};

// 一切准备就绪后，使用电子模块。
const { webFrameInit } = require('@electron/internal/renderer/web-frame-init') as typeof webFrameInitModule;
webFrameInit();

// 处理命令行参数。
const { hasSwitch, getSwitchValue } = process._linkedBinding('electron_common_command_line');
const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');

const contextIsolation = mainFrame.getWebPreference('contextIsolation');
const nodeIntegration = mainFrame.getWebPreference('nodeIntegration');
const webviewTag = mainFrame.getWebPreference('webviewTag');
const isHiddenPage = mainFrame.getWebPreference('hiddenPage');
const usesNativeWindowOpen = mainFrame.getWebPreference('nativeWindowOpen');
const preloadScript = mainFrame.getWebPreference('preload');
const preloadScripts = mainFrame.getWebPreference('preloadScripts');
const isWebView = mainFrame.getWebPreference('isWebView');
const openerId = mainFrame.getWebPreference('openerId');
const appPath = hasSwitch('app-path') ? getSwitchValue('app-path') : null;

// WebContents预加载脚本在会话预加载脚本之后加载。
if (preloadScript) {
  preloadScripts.push(preloadScript);
}

switch (window.location.protocol) {
  case 'devtools:': {
    // 覆盖一些检查器API。
    require('@electron/internal/renderer/inspector');
    break;
  }
  case 'chrome-extension:': {
    break;
  }
  case 'chrome:': {
    break;
  }
  default: {
    // 覆盖默认Web函数。
    const { windowSetup } = require('@electron/internal/renderer/window-setup') as typeof windowSetupModule;
    windowSetup(isWebView, openerId, isHiddenPage, usesNativeWindowOpen);
  }
}

// 加载WebView标记实现。
if (process.isMainFrame) {
  const { webViewInit } = require('@electron/internal/renderer/web-view/web-view-init') as typeof webViewInitModule;
  webViewInit(contextIsolation, webviewTag, isWebView);
}

if (nodeIntegration) {
  // 将节点绑定导出到全局。
  const { makeRequireFunction } = __non_webpack_require__('internal/modules/cjs/helpers') // ESRINT-DISABLE-LINE。
  global.module = new Module('electron/js2c/renderer_init');
  global.require = makeRequireFunction(global.module);

  // 如果是file：protocol，则将__filename设置为html文件的路径。
  if (window.location.protocol === 'file:') {
    const location = window.location;
    let pathname = location.pathname;

    if (process.platform === 'win32') {
      if (pathname[0] === '/') pathname = pathname.substr(1);

      const isWindowsNetworkSharePath = location.hostname.length > 0 && process.resourcesPath.startsWith('\\');
      if (isWindowsNetworkSharePath) {
        pathname = `// ${location.host}/${pathname}`；
      }
    }

    global.__filename = path.normalize(decodeURIComponent(pathname));
    global.__dirname = path.dirname(global.__filename);

    // 设置模块的文件名，以便相对要求可以按预期工作。
    global.module.filename = global.__filename;

    // 还可以在html文件下搜索模块。
    global.module.paths = Module._nodeModulePaths(global.__dirname);
  } else {
    // 为了向后兼容，我们在这里伪造了这两条路径。
    global.__filename = path.join(process.resourcesPath, 'electron.asar', 'renderer', 'init.js');
    global.__dirname = path.join(process.resourcesPath, 'electron.asar', 'renderer');

    if (appPath) {
      // 在app目录下搜索模块。
      global.module.paths = Module._nodeModulePaths(appPath);
    }
  }

  // 将window.onerror重定向到uncaughtException。
  window.onerror = function (_message, _filename, _lineno, _colno, error) {
    if (global.process.listenerCount('uncaughtException') > 0) {
      // 我们不想将`uncaughtException`添加到我们的定义中。
      // 因为我们不想让任何人(在任何地方)抛出这样的东西。
      // 犯了错误。
      global.process.emit('uncaughtException', error as any);
      return true;
    } else {
      return false;
    }
  };
} else {
  // 将环境加载到。
  // 非上下文隔离环境。
  if (!contextIsolation) {
    process.once('loaded', function () {
      delete (global as any).process;
      delete (global as any).Buffer;
      delete (global as any).setImmediate;
      delete (global as any).clearImmediate;
      delete (global as any).global;
      delete (global as any).root;
      delete (global as any).GLOBAL;
    });
  }
}

// 加载预加载脚本。
for (const preloadScript of preloadScripts) {
  try {
    Module._load(preloadScript);
  } catch (error) {
    console.error(`Unable to load preload script: ${preloadScript}`);
    console.error(error);

    ipcRendererInternal.send(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, preloadScript, error);
  }
}

// 警告安全问题
if (process.isMainFrame) {
  const { securityWarnings } = require('@electron/internal/renderer/security-warnings') as typeof securityWarningsModule;
  securityWarnings(nodeIntegration);
}
