import * as electron from 'electron';

import * as fs from 'fs';
import * as path from 'path';
import * as url from 'url';
const { app, dialog } = electron;

type DefaultAppOptions = {
  file: null | string;
  noHelp: boolean;
  version: boolean;
  webdriver: boolean;
  interactive: boolean;
  abi: boolean;
  modules: string[];
}

const Module = require('module');

// 解析命令行选项。
const argv = process.argv.slice(1);

const option: DefaultAppOptions = {
  file: null,
  noHelp: Boolean(process.env.ELECTRON_NO_HELP),
  version: false,
  webdriver: false,
  interactive: false,
  abi: false,
  modules: []
};

let nextArgIsRequire = false;

for (const arg of argv) {
  if (nextArgIsRequire) {
    option.modules.push(arg);
    nextArgIsRequire = false;
    continue;
  } else if (arg === '--version' || arg === '-v') {
    option.version = true;
    break;
  } else if (arg.match(/^--app=/)) {
    option.file = arg.split('=')[1];
    break;
  } else if (arg === '--interactive' || arg === '-i' || arg === '-repl') {
    option.interactive = true;
  } else if (arg === '--test-type=webdriver') {
    option.webdriver = true;
  } else if (arg === '--require' || arg === '-r') {
    nextArgIsRequire = true;
    continue;
  } else if (arg === '--abi' || arg === '-a') {
    option.abi = true;
    continue;
  } else if (arg === '--no-help') {
    option.noHelp = true;
    continue;
  } else if (arg[0] === '-') {
    continue;
  } else {
    option.file = arg;
    break;
  }
}

if (nextArgIsRequire) {
  console.error('Invalid Usage: --require [file]\n\n"file" is required');
  process.exit(1);
}

// 设置预加载模块。
if (option.modules.length > 0) {
  Module._preloadModules(option.modules);
}

function loadApplicationPackage (packagePath: string) {
  // 添加一个标志，指示应用程序是从默认应用程序启动的。
  Object.defineProperty(process, 'defaultApp', {
    configurable: false,
    enumerable: true,
    value: true
  });

  try {
    // 覆盖应用程序名称和版本。
    packagePath = path.resolve(packagePath);
    const packageJsonPath = path.join(packagePath, 'package.json');
    let appPath;
    if (fs.existsSync(packageJsonPath)) {
      let packageJson;
      try {
        packageJson = require(packageJsonPath);
      } catch (e) {
        showErrorMessage(`Unable to parse ${packageJsonPath}\n\n${e.message}`);
        return;
      }

      if (packageJson.version) {
        app.setVersion(packageJson.version);
      }
      if (packageJson.productName) {
        app.name = packageJson.productName;
      } else if (packageJson.name) {
        app.name = packageJson.name;
      }
      appPath = packagePath;
    }

    try {
      const filePath = Module._resolveFilename(packagePath, module, true);
      app.setAppPath(appPath || path.dirname(filePath));
    } catch (e) {
      showErrorMessage(`Unable to find Electron app at ${packagePath}\n\n${e.message}`);
      return;
    }

    // 运行应用程序。
    Module._load(packagePath, module, true);
  } catch (e) {
    console.error('App threw an error during load');
    console.error(e.stack || e);
    throw e;
  }
}

function showErrorMessage (message: string) {
  app.focus();
  dialog.showErrorBox('Error launching app', message);
  process.exit(1);
}

async function loadApplicationByURL (appUrl: string) {
  const { loadURL } = await import('./default_app');
  loadURL(appUrl);
}

async function loadApplicationByFile (appPath: string) {
  const { loadFile } = await import('./default_app');
  loadFile(appPath);
}

function startRepl () {
  if (process.platform === 'win32') {
    console.error('Electron REPL not currently supported on Windows');
    process.exit(1);
  }

  // 防止戒烟。
  app.on('window-all-closed', () => {});

  const GREEN = '32';
  const colorize = (color: string, s: string) => `\x1b[${color}m${s}\x1b[0m`;
  const electronVersion = colorize(GREEN, `v${process.versions.electron}`);
  const nodeVersion = colorize(GREEN, `v${process.versions.node}`);

  console.info(`
    Welcome to the Electron.js REPL \\[._.]/

    You can access all Electron.js modules here as well as Node.js modules.
    Using: Node.js ${nodeVersion} and Electron.js ${electronVersion}
  `);

  const { REPLServer } = require('repl');
  const repl = new REPLServer({
    prompt: '> '
  }).on('exit', () => {
    process.exit(0);
  });

  function defineBuiltin (context: any, name: string, getter: Function) {
    const setReal = (val: any) => {
      // 在重新分配属性之前删除该属性会禁用。
      // 吸气剂/凝固剂机构。
      delete context[name];
      context[name] = val;
    };

    Object.defineProperty(context, name, {
      get: () => {
        const lib = getter();

        delete context[name];
        Object.defineProperty(context, name, {
          get: () => lib,
          set: setReal,
          configurable: true,
          enumerable: false
        });

        return lib;
      },
      set: setReal,
      configurable: true,
      enumerable: false
    });
  }

  defineBuiltin(repl.context, 'electron', () => electron);
  for (const api of Object.keys(electron) as (keyof typeof electron)[]) {
    defineBuiltin(repl.context, api, () => electron[api]);
  }

  // 从node/lib/repl.js复制。为了更好的DX，我们不想。
  // 例如，以比“const”更高的优先级显示“contentTracing”，因此。
  // 我们仅在没有常用单词时触发自定义制表符完成。
  // 有可能匹配。
  const commonWords = [
    'async', 'await', 'break', 'case', 'catch', 'const', 'continue',
    'debugger', 'default', 'delete', 'do', 'else', 'export', 'false',
    'finally', 'for', 'function', 'if', 'import', 'in', 'instanceof', 'let',
    'new', 'null', 'return', 'switch', 'this', 'throw', 'true', 'try',
    'typeof', 'var', 'void', 'while', 'with', 'yield'
  ];

  const electronBuiltins = [...Object.keys(electron), 'original-fs', 'electron'];

  const defaultComplete = repl.completer;
  repl.completer = (line: string, callback: Function) => {
    const lastSpace = line.lastIndexOf(' ');
    const currentSymbol = line.substring(lastSpace + 1, repl.cursor);

    const filterFn = (c: string) => c.startsWith(currentSymbol);
    const ignores = commonWords.filter(filterFn);
    const hits = electronBuiltins.filter(filterFn);

    if (!ignores.length && hits.length) {
      callback(null, [hits, currentSymbol]);
    } else {
      defaultComplete.apply(repl, [line, callback]);
    }
  };
}

// 如果在命令行中指定了指定的应用程序，则启动该应用程序，否则。
// 启动默认应用程序。
if (option.file && !option.webdriver) {
  const file = option.file;
  const protocol = url.parse(file).protocol;
  const extension = path.extname(file);
  if (protocol === 'http:' || protocol === 'https:' || protocol === 'file:' || protocol === 'chrome:') {
    loadApplicationByURL(file);
  } else if (extension === '.html' || extension === '.htm') {
    loadApplicationByFile(path.resolve(file));
  } else {
    loadApplicationPackage(file);
  }
} else if (option.version) {
  console.log('v' + process.versions.electron);
  process.exit(0);
} else if (option.abi) {
  console.log(process.versions.modules);
  process.exit(0);
} else if (option.interactive) {
  startRepl();
} else {
  if (!option.noHelp) {
    const welcomeMessage = `
Electron ${process.versions.electron} - Build cross platform desktop apps with JavaScript, HTML, and CSS
Usage: electron [options] [path]

A path to an Electron app may be specified. It must be one of the following:
  - index.js file.
  - Folder containing a package.json file.
  - Folder containing an index.js file.
  - .html/.htm file.
  - http:// 、https://，或FILE：//url。

Options:
  -i, --interactive     Open a REPL to the main process.
  -r, --require         Module to preload (option can be repeated).
  -v, --version         Print the version.
  -a, --abi             Print the Node ABI version.`;

    console.log(welcomeMessage);
  }

  loadApplicationByFile('index.html');
}
