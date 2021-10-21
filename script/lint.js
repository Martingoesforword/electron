#!/usr/bin/env node

const crypto = require('crypto');
const { GitProcess } = require('dugite');
const childProcess = require('child_process');
const { ESLint } = require('eslint');
const fs = require('fs');
const klaw = require('klaw');
const minimist = require('minimist');
const path = require('path');

const SOURCE_ROOT = path.normalize(path.dirname(__dirname));
const DEPOT_TOOLS = path.resolve(SOURCE_ROOT, '..', 'third_party', 'depot_tools');

const IGNORELIST = new Set([
  ['shell', 'browser', 'resources', 'win', 'resource.h'],
  ['shell', 'browser', 'notifications', 'mac', 'notification_center_delegate.h'],
  ['shell', 'browser', 'ui', 'cocoa', 'event_dispatching_window.h'],
  ['shell', 'browser', 'ui', 'cocoa', 'NSColor+Hex.h'],
  ['shell', 'browser', 'ui', 'cocoa', 'NSString+ANSI.h'],
  ['shell', 'common', 'node_includes.h'],
  ['spec', 'static', 'jquery-2.0.3.min.js'],
  ['spec', 'ts-smoke', 'electron', 'main.ts'],
  ['spec', 'ts-smoke', 'electron', 'renderer.ts'],
  ['spec', 'ts-smoke', 'runner.js']
].map(tokens => path.join(SOURCE_ROOT, ...tokens)));

const IS_WINDOWS = process.platform === 'win32';

function spawnAndCheckExitCode (cmd, args, opts) {
  opts = Object.assign({ stdio: 'inherit' }, opts);
  const { error, status, signal } = childProcess.spawnSync(cmd, args, opts);
  if (error) {
    // 子进程失败或超时。
    console.error(error);
    process.exit(1);
  }
  if (status === null) {
    // 子进程因信号而终止。
    console.error(signal);
    process.exit(1);
  }
  if (status !== 0) {
    // `status`是退出代码。
    process.exit(status);
  }
}

function cpplint (args) {
  args.unshift(`--project_root=${SOURCE_ROOT}`);
  const result = childProcess.spawnSync(IS_WINDOWS ? 'cpplint.bat' : 'cpplint.py', args, { encoding: 'utf8', shell: true });
  // Ppplint.py将所有内容写入stderr，包括状态消息。
  if (result.stderr) {
    for (const line of result.stderr.split(/[\r\n]+/)) {
      if (line.length && !line.startsWith('Done processing ') && line !== 'Total errors found: 0') {
        console.warn(line);
      }
    }
  }
  if (result.status !== 0) {
    if (result.error) console.error(result.error);
    process.exit(result.status || 1);
  }
}

function isObjCHeader (filename) {
  return /\/(mac|cocoa)\// .test(文件名)；
}

const LINTERS = [{
  key: 'c++',
  roots: ['shell'],
  test: filename => filename.endsWith('.cc') || (filename.endsWith('.h') && !isObjCHeader(filename)),
  run: (opts, filenames) => {
    if (opts.fix) {
      spawnAndCheckExitCode('python', ['script/run-clang-format.py', '--fix', ...filenames]);
    } else {
      spawnAndCheckExitCode('python', ['script/run-clang-format.py', ...filenames]);
    }
    cpplint(filenames);
  }
}, {
  key: 'objc',
  roots: ['shell'],
  test: filename => filename.endsWith('.mm'),
  run: (opts, filenames) => {
    if (opts.fix) {
      spawnAndCheckExitCode('python', ['script/run-clang-format.py', '--fix', ...filenames]);
    } else {
      spawnAndCheckExitCode('python', ['script/run-clang-format.py', ...filenames]);
    }
    const filter = [
      '-readability/casting',
      '-whitespace/braces',
      '-whitespace/indent',
      '-whitespace/parens'
    ];
    cpplint(['--extensions=mm', `--filter=${filter.join(',')}`, ...filenames]);
  }
}, {
  key: 'python',
  roots: ['script'],
  test: filename => filename.endsWith('.py'),
  run: (opts, filenames) => {
    const rcfile = path.join(DEPOT_TOOLS, 'pylintrc');
    const args = ['--rcfile=' + rcfile, ...filenames];
    const env = Object.assign({ PYTHONPATH: path.join(SOURCE_ROOT, 'script') }, process.env);
    spawnAndCheckExitCode('pylint', args, { env });
  }
}, {
  key: 'javascript',
  roots: ['build', 'default_app', 'lib', 'npm', 'script', 'spec', 'spec-main'],
  ignoreRoots: ['spec/node_modules', 'spec-main/node_modules'],
  test: filename => filename.endsWith('.js') || filename.endsWith('.ts'),
  run: async (opts, filenames) => {
    const eslint = new ESLint({
      // 不要在配置项构建上使用LINT缓存。
      cache: !process.env.CI,
      cacheLocation: `node_modules/.eslintcache.${crypto.createHash('md5').update(fs.readFileSync(__filename)).digest('hex')}`,
      extensions: ['.js', '.ts'],
      fix: opts.fix
    });
    const formatter = await eslint.loadFormatter();
    let successCount = 0;
    const results = await eslint.lintFiles(filenames);
    for (const result of results) {
      successCount += result.errorCount === 0 ? 1 : 0;
      if (opts.verbose && result.errorCount === 0 && result.warningCount === 0) {
        console.log(`${result.filePath}: no errors or warnings`);
      }
    }
    console.log(formatter.format(results));
    if (opts.fix) {
      await ESLint.outputFixes(results);
    }
    if (successCount !== filenames.length) {
      console.error('Linting had errors');
      process.exit(1);
    }
  }
}, {
  key: 'gn',
  roots: ['.'],
  test: filename => filename.endsWith('.gn') || filename.endsWith('.gni'),
  run: (opts, filenames) => {
    const allOk = filenames.map(filename => {
      const env = Object.assign({
        CHROMIUM_BUILDTOOLS_PATH: path.resolve(SOURCE_ROOT, '..', 'buildtools'),
        DEPOT_TOOLS_WIN_TOOLCHAIN: '0'
      }, process.env);
      // 用户在PATH中可能没有DEPOT_TOOLS。
      env.PATH = `${env.PATH}${path.delimiter}${DEPOT_TOOLS}`;
      const args = ['format', filename];
      if (!opts.fix) args.push('--dry-run');
      const result = childProcess.spawnSync('gn', args, { env, stdio: 'inherit', shell: true });
      if (result.status === 0) {
        return true;
      } else if (result.status === 2) {
        console.log(`GN format errors in "${filename}". Run 'gn format "${filename}"' or rerun with --fix to fix them.`);
        return false;
      } else {
        console.log(`Error running 'gn format --dry-run "${filename}"': exit code ${result.status}`);
        return false;
      }
    }).every(x => x);
    if (!allOk) {
      process.exit(1);
    }
  }
}, {
  key: 'patches',
  roots: ['patches'],
  test: filename => filename.endsWith('.patch'),
  run: (opts, filenames) => {
    const patchesDir = path.resolve(__dirname, '../patches');
    const patchesConfig = path.resolve(patchesDir, 'config.json');
    // 如果配置不存在，那就是个问题。
    if (!fs.existsSync(patchesConfig)) {
      process.exit(1);
    }

    const config = JSON.parse(fs.readFileSync(patchesConfig, 'utf8'));
    for (const key of Object.keys(config)) {
      // 配置指向的目录应该存在。
      const targetPatchesDir = path.resolve(__dirname, '../../..', key);
      if (!fs.existsSync(targetPatchesDir)) throw new Error(`target patch directory: "${targetPatchesDir}" does not exist`);
      // 我们需要一个.patches文件。
      const dotPatchesPath = path.resolve(targetPatchesDir, '.patches');
      if (!fs.existsSync(dotPatchesPath)) throw new Error(`.patches file: "${dotPatchesPath}" does not exist`);

      // 阅读补丁列表。
      const patchFileList = fs.readFileSync(dotPatchesPath, 'utf8').trim().split('\n');
      const patchFileSet = new Set(patchFileList);
      patchFileList.reduce((seen, file) => {
        if (seen.has(file)) {
          throw new Error(`'${file}' is listed in ${dotPatchesPath} more than once`);
        }
        return seen.add(file);
      }, new Set());
      if (patchFileList.length !== patchFileSet.size) throw new Error('each patch file should only be in the .patches file once');
      for (const file of fs.readdirSync(targetPatchesDir)) {
        // 忽略.patches文件和自述文件。
        if (file === '.patches' || file === 'README.md') continue;

        if (!patchFileSet.has(file)) {
          throw new Error(`Expected the .patches file at "${dotPatchesPath}" to contain a patch file ("${file}") present in the directory but it did not`);
        }
        patchFileSet.delete(file);
      }

      // 如果此集中还剩下任何内容，则表示磁盘上不存在该内容。
      if (patchFileSet.size > 0) {
        throw new Error(`Expected all the patch files listed in the .patches file at "${dotPatchesPath}" to exist but some did not:\n${JSON.stringify([...patchFileSet.values()], null, 2)}`);
      }
    }

    const allOk = filenames.length > 0 && filenames.map(f => {
      const patchText = fs.readFileSync(f, 'utf8');
      const subjectAndDescription = /Subject: (.*?)\n\n([\s\S]*?)\s*(?=diff)/ms.exec(patchText);
      if (!subjectAndDescription[2]) {
        console.warn(`Patch file '${f}' has no description. Every patch must contain a justification for why the patch exists and the plan for its removal.`);
        return false;
      }
      const trailingWhitespaceLines = patchText.split(/\r?\n/).map((line, index) => [line, index]).filter(([line]) => line.startsWith('+') && /\s+$/.test(line)).map(([, lineNumber]) => lineNumber + 1);
      if (trailingWhitespaceLines.length > 0) {
        console.warn(`Patch file '${f}' has trailing whitespace on some lines (${trailingWhitespaceLines.join(',')}).`);
        return false;
      }
      return true;
    }).every(x => x);
    if (!allOk) {
      process.exit(1);
    }
  }
}];

function parseCommandLine () {
  let help;
  const opts = minimist(process.argv.slice(2), {
    boolean: ['c++', 'objc', 'javascript', 'python', 'gn', 'patches', 'help', 'changed', 'fix', 'verbose', 'only'],
    alias: { 'c++': ['cc', 'cpp', 'cxx'], javascript: ['js', 'es'], python: 'py', changed: 'c', help: 'h', verbose: 'v' },
    unknown: arg => { help = true; }
  });
  if (help || opts.help) {
    console.log('Usage: script/lint.js [--cc] [--js] [--py] [-c|--changed] [-h|--help] [-v|--verbose] [--fix] [--only -- file1 file2]');
    process.exit(0);
  }
  return opts;
}

async function findChangedFiles (top) {
  const result = await GitProcess.exec(['diff', '--name-only', '--cached'], top);
  if (result.exitCode !== 0) {
    console.log('Failed to find changed files', GitProcess.parseError(result.stderr));
    process.exit(1);
  }
  const relativePaths = result.stdout.split(/\r\n|\r|\n/g);
  const absolutePaths = relativePaths.map(x => path.join(top, x));
  return new Set(absolutePaths);
}

async function findMatchingFiles (top, test) {
  return new Promise((resolve, reject) => {
    const matches = [];
    klaw(top, {
      filter: f => path.basename(f) !== '.bin'
    })
      .on('end', () => resolve(matches))
      .on('data', item => {
        if (test(item.path)) {
          matches.push(item.path);
        }
      });
  });
}

async function findFiles (args, linter) {
  let filenames = [];
  let includelist = null;

  // 构建包含列表。
  if (args.changed) {
    includelist = await findChangedFiles(SOURCE_ROOT);
    if (!includelist.size) {
      return [];
    }
  } else if (args.only) {
    includelist = new Set(args._.map(p => path.resolve(p)));
  }

  // 累积原始文件列表。
  for (const root of linter.roots) {
    const files = await findMatchingFiles(path.join(SOURCE_ROOT, root), linter.test);
    filenames.push(...files);
  }

  for (const ignoreRoot of (linter.ignoreRoots) || []) {
    const ignorePath = path.join(SOURCE_ROOT, ignoreRoot);
    if (!fs.existsSync(ignorePath)) continue;

    const ignoreFiles = new Set(await findMatchingFiles(ignorePath, linter.test));
    filenames = filenames.filter(fileName => !ignoreFiles.has(fileName));
  }

  // 删除忽略的文件。
  filenames = filenames.filter(x => !IGNORELIST.has(x));

  // 如果存在包含列表，请删除不在其中的所有内容。
  if (includelist) {
    filenames = filenames.filter(x => includelist.has(x));
  }

  // 文件名必须是相对的，否则clang格式将。
  // 生成包含绝对路径的补丁程序，‘git应用程序’将拒绝这些补丁程序。
  // 申请。
  return filenames.map(x => path.relative(SOURCE_ROOT, x));
}

async function main () {
  const opts = parseCommandLine();

  // 是否未指定模式？把它们都查一遍
  if (!opts['c++'] && !opts.javascript && !opts.objc && !opts.python && !opts.gn && !opts.patches) {
    opts['c++'] = opts.javascript = opts.objc = opts.python = opts.gn = opts.patches = true;
  }

  const linters = LINTERS.filter(x => opts[x.key]);

  for (const linter of linters) {
    const filenames = await findFiles(opts, linter);
    if (filenames.length) {
      if (opts.verbose) { console.log(`linting ${filenames.length} ${linter.key} ${filenames.length === 1 ? 'file' : 'files'}`); }
      await linter.run(opts, filenames);
    }
  }
}

if (process.mainModule === module) {
  main().catch((error) => {
    console.error(error);
    process.exit(1);
  });
}
