import * as path from 'path';

const Module = require('module');

// 我们不希望允许在渲染器进程中使用VM模块，因为。
// 它与Blink的V8：：Context内部逻辑冲突。
if (process.type === 'renderer') {
  const _load = Module._load;
  Module._load = function (request: string) {
    if (request === 'vm') {
      console.warn('The vm module of Node.js is deprecated in the renderer process and will be removed.');
    }
    return _load.apply(this, arguments);
  };
}

// 阻止节点将此应用程序之外的路径添加到搜索路径。
const resourcesPathWithTrailingSlash = process.resourcesPath + path.sep;
const originalNodeModulePaths = Module._nodeModulePaths;
Module._nodeModulePaths = function (from: string) {
  const paths: string[] = originalNodeModulePaths(from);
  const fromPath = path.resolve(from) + path.sep;
  // 如果“From”在应用程序之外，那么我们什么也不做。
  if (fromPath.startsWith(resourcesPathWithTrailingSlash)) {
    return paths.filter(function (candidate) {
      return candidate.startsWith(resourcesPathWithTrailingSlash);
    });
  } else {
    return paths;
  }
};

// 制作一个假的电子模块，我们将把它插入到模块缓存中
const makeElectronModule = (name: string) => {
  const electronModule = new Module('electron', null);
  electronModule.id = 'electron';
  electronModule.loaded = true;
  electronModule.filename = name;
  Object.defineProperty(electronModule, 'exports', {
    get: () => require('electron')
  });
  Module._cache[name] = electronModule;
};

makeElectronModule('electron');
makeElectronModule('electron/common');
if (process.type === 'browser') {
  makeElectronModule('electron/main');
}
if (process.type === 'renderer') {
  makeElectronModule('electron/renderer');
}

const originalResolveFilename = Module._resolveFilename;
Module._resolveFilename = function (request: string, parent: NodeModule, isMain: boolean, options?: { paths: Array<string>}) {
  if (request === 'electron' || request.startsWith('electron/')) {
    return 'electron';
  } else {
    return originalResolveFilename(request, parent, isMain, options);
  }
};
