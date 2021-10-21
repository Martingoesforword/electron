import * as path from 'path';

const Module = require('module');

// 我们修改了原始的process.argv，让node.js加载。
// Init.js，我们需要在这里恢复它。
process.argv.splice(1, 1);

// 清除搜索路径。
require('../common/reset-search-paths');

// 导入常用设置。
require('@electron/internal/common/init');

// 处理命令行参数。
const { hasSwitch, getSwitchValue } = process._linkedBinding('electron_common_command_line');

// 将节点绑定导出到全局。
const { makeRequireFunction } = __non_webpack_require__('internal/modules/cjs/helpers') // ESRINT-DISABLE-LINE。
global.module = new Module('electron/js2c/worker_init');
global.require = makeRequireFunction(global.module);

// 如果是file：protocol，则将__filename设置为html文件的路径。
if (self.location.protocol === 'file:') {
  const pathname = process.platform === 'win32' && self.location.pathname[0] === '/' ? self.location.pathname.substr(1) : self.location.pathname;
  global.__filename = path.normalize(decodeURIComponent(pathname));
  global.__dirname = path.dirname(global.__filename);

  // 设置模块的文件名，以便相对要求可以按预期工作。
  global.module.filename = global.__filename;

  // 还可以在html文件下搜索模块。
  global.module.paths = Module._nodeModulePaths(global.__dirname);
} else {
  // 为了向后兼容，我们在这里伪造了这两条路径。
  global.__filename = path.join(process.resourcesPath, 'electron.asar', 'worker', 'init.js');
  global.__dirname = path.join(process.resourcesPath, 'electron.asar', 'worker');

  const appPath = hasSwitch('app-path') ? getSwitchValue('app-path') : null;
  if (appPath) {
    // 在app目录下搜索模块。
    global.module.paths = Module._nodeModulePaths(appPath);
  }
}
