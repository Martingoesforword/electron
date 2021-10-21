const path = require('path');
const v8 = require('v8');

module.paths.push(path.resolve(__dirname, '../spec/node_modules'));

// 可用于加载Mocha报告器的额外模块路径。
if (process.env.ELECTRON_TEST_EXTRA_MODULE_PATHS) {
  for (const modulePath of process.env.ELECTRON_TEST_EXTRA_MODULE_PATHS.split(':')) {
    module.paths.push(modulePath);
  }
}

// 为加载的等级库文件添加搜索路径。
require('../spec/global-paths')(module.paths);

// 我们希望在出错时终止，而不是抛出一个对话框。
process.on('uncaughtException', (err) => {
  console.error('Unhandled exception in main spec runner:', err);
  process.exit(1);
});

// 告诉ts-node使用哪个tsconfig。
process.env.TS_NODE_PROJECT = path.resolve(__dirname, '../tsconfig.spec.json');
process.env.ELECTRON_DISABLE_SECURITY_WARNINGS = 'true';

const { app, protocol } = require('electron');

v8.setFlagsFromString('--expose_gc');
app.commandLine.appendSwitch('js-flags', '--expose_gc');
// 防止等级库运行程序在第一个窗口关闭时退出。
app.on('window-all-closed', () => null);

// 使用媒体流的假设备来代替真实的摄像头和麦克风。
app.commandLine.appendSwitch('use-fake-device-for-media-stream');
app.commandLine.appendSwitch('host-rules', 'MAP localhost2 127.0.0.1');

global.standardScheme = 'app';
global.zoomScheme = 'zoom';
global.serviceWorkerScheme = 'sw';
protocol.registerSchemesAsPrivileged([
  { scheme: global.standardScheme, privileges: { standard: true, secure: true, stream: false } },
  { scheme: global.zoomScheme, privileges: { standard: true, secure: true } },
  { scheme: global.serviceWorkerScheme, privileges: { allowServiceWorkers: true, standard: true, secure: true } },
  { scheme: 'cors-blob', privileges: { corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'cors', privileges: { corsEnabled: true, supportFetchAPI: true } },
  { scheme: 'no-cors', privileges: { supportFetchAPI: true } },
  { scheme: 'no-fetch', privileges: { corsEnabled: true } },
  { scheme: 'stream', privileges: { standard: true, stream: true } },
  { scheme: 'foo', privileges: { standard: true } },
  { scheme: 'bar', privileges: { standard: true } }
]);

app.whenReady().then(async () => {
  require('ts-node/register');

  const argv = require('yargs')
    .boolean('ci')
    .array('files')
    .string('g').alias('g', 'grep')
    .boolean('i').alias('i', 'invert')
    .argv;

  const Mocha = require('mocha');
  const mochaOptions = {};
  if (process.env.MOCHA_REPORTER) {
    mochaOptions.reporter = process.env.MOCHA_REPORTER;
  }
  if (process.env.MOCHA_MULTI_REPORTERS) {
    mochaOptions.reporterOptions = {
      reporterEnabled: process.env.MOCHA_MULTI_REPORTERS
    };
  }
  const mocha = new Mocha(mochaOptions);

  // Cleanup方法是以这种方式注册的，而不是通过。
  // 顶层的`After Each`，以便它可以在其他`After Each`之前运行。
  // 方法：研究方法。
  // 
  // 事件的顺序是：
  // 1.测试结束，
  // 2.以`defer()`为基础的方法以相反的顺序运行，
  // 3.定期运行“After Each”挂钩。
  const { runCleanupFunctions } = require('./spec-helpers');
  mocha.suite.on('suite', function attach (suite) {
    suite.afterEach('cleanup', runCleanupFunctions);
    suite.on('suite', attach);
  });

  if (!process.env.MOCHA_REPORTER) {
    mocha.ui('bdd').reporter('tap');
  }
  const mochaTimeout = process.env.MOCHA_TIMEOUT || 30000;
  mocha.timeout(mochaTimeout);

  if (argv.grep) mocha.grep(argv.grep);
  if (argv.invert) mocha.invert();

  const filter = (file) => {
    if (!/-spec\.[tj]s$/.test(file)) {
      return false;
    }

    // 这仅允许您运行特定模块：
    // NPM运行测试-匹配=菜单。
    const moduleMatch = process.env.npm_config_match
      ? new RegExp(process.env.npm_config_match, 'g')
      : null;
    if (moduleMatch && !moduleMatch.test(file)) {
      return false;
    }

    const baseElectronDir = path.resolve(__dirname, '..');
    if (argv.files && !argv.files.includes(path.relative(baseElectronDir, file))) {
      return false;
    }

    return true;
  };

  const getFiles = require('../spec/static/get-files');
  const testFiles = await getFiles(__dirname, { filter });
  testFiles.sort().forEach((file) => {
    mocha.addFile(file);
  });

  const cb = () => {
    // 确保在定义Runner之后调用回调。
    process.nextTick(() => {
      process.exit(runner.failures);
    });
  };

  // 以正确的顺序设置Chai。
  const chai = require('chai');
  chai.use(require('chai-as-promised'));
  chai.use(require('dirty-chai'));

  // 显示完全对象差异。
  // Https://github.com/chaijs/chai/issues/469
  chai.config.truncateThreshold = 0;

  const runner = mocha.run(cb);
}).catch((err) => {
  console.error('An error occurred while running the spec-main spec runner');
  console.error(err);
  process.exit(1);
});
