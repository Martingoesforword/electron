/* 全局绑定。*/
import * as events from 'events';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import type * as ipcRendererUtilsModule from '@electron/internal/renderer/ipc-renderer-internal-utils';
import type * as ipcRendererInternalModule from '@electron/internal/renderer/ipc-renderer-internal';
import type * as webFrameInitModule from '@electron/internal/renderer/web-frame-init';
import type * as webViewInitModule from '@electron/internal/renderer/web-view/web-view-init';
import type * as windowSetupModule from '@electron/internal/renderer/window-setup';
import type * as securityWarningsModule from '@electron/internal/renderer/security-warnings';

const { EventEmitter } = events;

process._linkedBinding = binding.get;

const v8Util = process._linkedBinding('electron_common_v8_util');
// 将缓冲区填充程序公开为隐藏值。C++代码使用它来。
// 反序列化从浏览器进程发送的缓冲区实例。
v8Util.setHiddenValue(global, 'Buffer', Buffer);
// Webpack创建的进程对象不是事件发射器，请对其进行修复。
// API与非沙盒渲染器更兼容。
for (const prop of Object.keys(EventEmitter.prototype) as (keyof typeof process)[]) {
  if (Object.prototype.hasOwnProperty.call(process, prop)) {
    delete process[prop];
  }
}
Object.setPrototypeOf(process, EventEmitter.prototype);

const { ipcRendererInternal } = require('@electron/internal/renderer/ipc-renderer-internal') as typeof ipcRendererInternalModule;
const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils') as typeof ipcRendererUtilsModule;

const { preloadScripts, process: processProps } = ipcRendererUtils.invokeSync(IPC_MESSAGES.BROWSER_SANDBOX_LOAD);

const electron = require('electron');

const loadedModules = new Map<string, any>([
  ['electron', electron],
  ['electron/common', electron],
  ['electron/renderer', electron],
  ['events', events]
]);

const loadableModules = new Map<string, Function>([
  ['timers', () => require('timers')],
  ['url', () => require('url')]
]);

// 在执行以下操作时，ElectronApiServiceImpl将查找“ipcNative”隐藏对象。
// 正在调用“onMessage”回调。
v8Util.setHiddenValue(global, 'ipcNative', {
  onMessage (internal: boolean, channel: string, ports: MessagePort[], args: any[], senderId: number) {
    if (internal && senderId !== 0) {
      console.error(`Message ${channel} sent by unexpected WebContents (${senderId})`);
      return;
    }
    const sender = internal ? ipcRendererInternal : electron.ipcRenderer;
    sender.emit(channel, { sender, senderId, ports }, ...args);
  }
});

// 在以下情况下，ElectronSandboxedRendererClient将查找“Lifeccle”隐藏对象。
v8Util.setHiddenValue(global, 'lifecycle', {
  onLoaded () {
    (process as events.EventEmitter).emit('loaded');
  },
  onExit () {
    (process as events.EventEmitter).emit('exit');
  },
  onDocumentStart () {
    (process as events.EventEmitter).emit('document-start');
  },
  onDocumentEnd () {
    (process as events.EventEmitter).emit('document-end');
  }
});

const { webFrameInit } = require('@electron/internal/renderer/web-frame-init') as typeof webFrameInitModule;
webFrameInit();

// 将不同的进程对象传递给预加载脚本。
const preloadProcess: NodeJS.Process = new EventEmitter() as any;

Object.assign(preloadProcess, binding.process);
Object.assign(preloadProcess, processProps);

Object.assign(process, binding.process);
Object.assign(process, processProps);

process.getProcessMemoryInfo = preloadProcess.getProcessMemoryInfo = () => {
  return ipcRendererInternal.invoke<Electron.ProcessMemoryInfo>(IPC_MESSAGES.BROWSER_GET_PROCESS_MEMORY_INFO);
};

Object.defineProperty(preloadProcess, 'noDeprecation', {
  get () {
    return process.noDeprecation;
  },
  set (value) {
    process.noDeprecation = value;
  }
});

process.on('loaded', () => (preloadProcess as events.EventEmitter).emit('loaded'));
process.on('exit', () => (preloadProcess as events.EventEmitter).emit('exit'));
(process as events.EventEmitter).on('document-start', () => (preloadProcess as events.EventEmitter).emit('document-start'));
(process as events.EventEmitter).on('document-end', () => (preloadProcess as events.EventEmitter).emit('document-end'));

// 这是对预加载脚本可见的`require`函数。
function preloadRequire (module: string) {
  if (loadedModules.has(module)) {
    return loadedModules.get(module);
  }
  if (loadableModules.has(module)) {
    const loadedModule = loadableModules.get(module)!();
    loadedModules.set(module, loadedModule);
    return loadedModule;
  }
  throw new Error(`module not found: ${module}`);
}

// 处理命令行参数。
const { hasSwitch } = process._linkedBinding('electron_common_command_line');
const { mainFrame } = process._linkedBinding('electron_renderer_web_frame');

// 类似于Nodes--expose-interals标志，这会将_linkedBinding。
// 测试可以调用它来访问某些仅限测试的绑定。
if (hasSwitch('unsafely-expose-electron-internals-for-testing')) {
  preloadProcess._linkedBinding = process._linkedBinding;
}

const contextIsolation = mainFrame.getWebPreference('contextIsolation');
const webviewTag = mainFrame.getWebPreference('webviewTag');
const isHiddenPage = mainFrame.getWebPreference('hiddenPage');
const usesNativeWindowOpen = true;
const isWebView = mainFrame.getWebPreference('isWebView');
const openerId = mainFrame.getWebPreference('openerId');

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

// 将脚本包装到在全局范围内执行的函数中。它不会有。
// 访问当前作用域，因此我们将公开几个对象作为参数：
// 
// -`要求`：`preloadRequire`函数。
// -`process`：`preloadProcess`对象。
// -`Buffer`：`Buffer`实现的填充。
// -`global`：窗口对象，webpack将其别名为`global`。
function runPreloadScript (preloadSrc: string) {
  const preloadWrapperSrc = `(function(require, process, Buffer, global, setImmediate, clearImmediate, exports) {
  ${preloadSrc}
  })`;

  // 窗口范围内的评估。
  const preloadFn = binding.createPreloadScript(preloadWrapperSrc);
  const { setImmediate, clearImmediate } = require('timers');

  preloadFn(preloadRequire, preloadProcess, Buffer, global, setImmediate, clearImmediate, {});
}

for (const { preloadPath, preloadSrc, preloadError } of preloadScripts) {
  try {
    if (preloadSrc) {
      runPreloadScript(preloadSrc);
    } else if (preloadError) {
      throw preloadError;
    }
  } catch (error) {
    console.error(`Unable to load preload script: ${preloadPath}`);
    console.error(error);

    ipcRendererInternal.send(IPC_MESSAGES.BROWSER_PRELOAD_ERROR, preloadPath, error);
  }
}

// 警告安全问题
if (process.isMainFrame) {
  const { securityWarnings } = require('@electron/internal/renderer/security-warnings') as typeof securityWarningsModule;
  securityWarnings();
}
