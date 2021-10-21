import chalk from 'chalk';
import * as childProcess from 'child_process';
import * as fs from 'fs';
import * as klaw from 'klaw';
import * as minimist from 'minimist';
import * as os from 'os';
import * as path from 'path';
import * as streamChain from 'stream-chain';
import * as streamJson from 'stream-json';
import { ignore as streamJsonIgnore } from 'stream-json/filters/Ignore';
import { streamArray as streamJsonStreamArray } from 'stream-json/streamers/StreamArray';

const SOURCE_ROOT = path.normalize(path.dirname(__dirname));
const LLVM_BIN = path.resolve(
  SOURCE_ROOT,
  '..',
  'third_party',
  'llvm-build',
  'Release+Asserts',
  'bin'
);
const PLATFORM = os.platform();

type SpawnAsyncResult = {
  stdout: string;
  stderr: string;
  status: number | null;
};

class ErrorWithExitCode extends Error {
  exitCode: number;

  constructor (message: string, exitCode: number) {
    super(message);
    this.exitCode = exitCode;
  }
}

async function spawnAsync (
  command: string,
  args: string[],
  options?: childProcess.SpawnOptionsWithoutStdio | undefined
): Promise<SpawnAsyncResult> {
  return new Promise((resolve, reject) => {
    try {
      const stdio = { stdout: '', stderr: '' };
      const spawned = childProcess.spawn(command, args, options || {});

      spawned.stdout.on('data', (data) => {
        stdio.stdout += data;
      });

      spawned.stderr.on('data', (data) => {
        stdio.stderr += data;
      });

      spawned.on('exit', (code) => resolve({ ...stdio, status: code }));
      spawned.on('error', (err) => reject(err));
    } catch (err) {
      reject(err);
    }
  });
}

function getDepotToolsEnv (): NodeJS.ProcessEnv {
  let depotToolsEnv;

  const findDepotToolsOnPath = () => {
    const result = childProcess.spawnSync(
      PLATFORM === 'win32' ? 'where' : 'which',
      ['gclient']
    );

    if (result.status === 0) {
      return process.env;
    }
  };

  const checkForBuildTools = () => {
    const result = childProcess.spawnSync(
      'electron-build-tools',
      ['show', 'env', '--json'],
      { shell: true }
    );

    if (result.status === 0) {
      return {
        ...process.env,
        ...JSON.parse(result.stdout.toString().trim())
      };
    }
  };

  try {
    depotToolsEnv = findDepotToolsOnPath();
    if (!depotToolsEnv) depotToolsEnv = checkForBuildTools();
  } catch {}

  if (!depotToolsEnv) {
    throw new Error("Couldn't find depot_tools, ensure it's on your PATH");
  }

  if (!('CHROMIUM_BUILDTOOLS_PATH' in depotToolsEnv)) {
    throw new Error(
      'CHROMIUM_BUILDTOOLS_PATH environment variable must be set'
    );
  }

  return depotToolsEnv;
}

function chunkFilenames (filenames: string[], offset: number = 0): string[][] {
  // Windows的最大命令行长度为2047个字符，因此我们无法。
  // 提供太多的文件名，而不仔细查看。为了解决这个问题，
  // 将文件名列表分块，以便在以下情况下不会超过该限制。
  // 用作参数。在其他平台上使用更高的限制，这将。
  // 实际上是个禁区。
  const MAX_FILENAME_ARGS_LENGTH =
    PLATFORM === 'win32' ? 2047 - offset : 100 * 1024;

  return filenames.reduce(
    (chunkedFilenames: string[][], filename) => {
      const currChunk = chunkedFilenames[chunkedFilenames.length - 1];
      const currChunkLength = currChunk.reduce(
        (totalLength, _filename) => totalLength + _filename.length + 1,
        0
      );
      if (currChunkLength + filename.length + 1 > MAX_FILENAME_ARGS_LENGTH) {
        chunkedFilenames.push([filename]);
      } else {
        currChunk.push(filename);
      }
      return chunkedFilenames;
    },
    [[]]
  );
}

async function runClangTidy (
  outDir: string,
  filenames: string[],
  checks: string = '',
  jobs: number = 1
): Promise<boolean> {
  const cmd = path.resolve(LLVM_BIN, 'clang-tidy');
  const args = [`-p=${outDir}`];

  if (checks) args.push(`--checks=${checks}`);

  // 删除所有不在编译数据库中的文件，以防止。
  // 把输出搞得乱七八糟的错误。由于编译数据库有数百个。
  // 以兆字节为单位，这是通过流式传输来完成的，这样就不会将其全部保存在内存中。
  const filterCompilationDatabase = (): Promise<string[]> => {
    const compiledFilenames: string[] = [];

    return new Promise((resolve) => {
      const pipeline = streamChain.chain([
        fs.createReadStream(path.resolve(outDir, 'compile_commands.json')),
        streamJson.parser(),
        streamJsonIgnore({ filter: /\bcommand\b/i }),
        streamJsonStreamArray(),
        ({ value: { file, directory } }) => {
          const filename = path.resolve(directory, file);
          return filenames.includes(filename) ? filename : null;
        }
      ]);

      pipeline.on('data', (data) => compiledFilenames.push(data));
      pipeline.on('end', () => resolve(compiledFilenames));
    });
  };

  // Clang-tidy可以从短的相对文件名中找出文件，所以。
  // 为了在命令行上获得最大效益，让我们修剪。
  // 将文件名减少到最少，以便我们可以在每次调用时容纳更多文件名。
  filenames = (await filterCompilationDatabase()).map((filename) =>
    path.relative(SOURCE_ROOT, filename)
  );

  if (filenames.length === 0) {
    throw new Error('No filenames to run');
  }

  const commandLength =
    cmd.length + args.reduce((length, arg) => length + arg.length, 0);

  const results: boolean[] = [];
  const asyncWorkers = [];
  const chunkedFilenames: string[][] = [];

  const filesPerWorker = Math.ceil(filenames.length / jobs);

  for (let i = 0; i < jobs; i++) {
    chunkedFilenames.push(
      ...chunkFilenames(filenames.splice(0, filesPerWorker), commandLength)
    );
  }

  const worker = async () => {
    let filenames = chunkedFilenames.shift();

    while (filenames) {
      results.push(
        await spawnAsync(cmd, [...args, ...filenames], {}).then((result) => {
          // 我们掉了颜色，所以重新上色吧，因为它更容易辨认。
          // 有一个--使用颜色的旗帜来表示叮当--整洁，但它没有任何效果。
          // 目前在Windows上，所以给每个人重新上色就好了。
          let state = null;

          for (const line of result.stdout.split('\n')) {
            if (line.includes(' warning: ')) {
              console.log(
                line
                  .split(' warning: ')
                  .map((part) => chalk.whiteBright(part))
                  .join(chalk.magentaBright(' warning: '))
              );
              state = 'code-line';
            } else if (line.includes(' note: ')) {
              const lineParts = line.split(' note: ');
              lineParts[0] = chalk.whiteBright(lineParts[0]);
              console.log(lineParts.join(chalk.grey(' note: ')));
              state = 'code-line';
            } else if (line.startsWith('error:')) {
              console.log(
                chalk.redBright('error: ') + line.split(' ').slice(1).join(' ')
              );
            } else if (state === 'code-line') {
              console.log(line);
              state = 'post-code-line';
            } else if (state === 'post-code-line') {
              console.log(chalk.greenBright(line));
            } else {
              console.log(line);
            }
          }

          if (result.status !== 0) {
            console.error(result.stderr);
          }

          // 在一次干净的运行中，stdout上没有任何内容。仅包含警告的运行。
          // 的状态代码为零，但在stdout上有输出。
          return result.status === 0 && result.stdout.length === 0;
        })
      );

      filenames = chunkedFilenames.shift();
    }
  };

  for (let i = 0; i < jobs; i++) {
    asyncWorkers.push(worker());
  }

  try {
    await Promise.all(asyncWorkers);
    return results.every((x) => x);
  } catch {
    return false;
  }
}

async function findMatchingFiles (
  top: string,
  test: (filename: string) => boolean
): Promise<string[]> {
  return new Promise((resolve) => {
    const matches = [] as string[];
    klaw(top, {
      filter: (f) => path.basename(f) !== '.bin'
    })
      .on('end', () => resolve(matches))
      .on('data', (item) => {
        if (test(item.path)) {
          matches.push(item.path);
        }
      });
  });
}

function parseCommandLine () {
  const showUsage = (arg?: string) : boolean => {
    if (!arg || arg.startsWith('-')) {
      console.log(
        'Usage: script/run-clang-tidy.ts [-h|--help] [--jobs|-j] ' +
          '[--checks] --out-dir OUTDIR [file1 file2]'
      );
      process.exit(0);
    }

    return true;
  };

  const opts = minimist(process.argv.slice(2), {
    boolean: ['help'],
    string: ['checks', 'out-dir'],
    default: { jobs: 1 },
    alias: { help: 'h', jobs: 'j' },
    stopEarly: true,
    unknown: showUsage
  });

  if (opts.help) showUsage();

  if (!opts['out-dir']) {
    console.log('--out-dir is a required argunment');
    process.exit(0);
  }

  return opts;
}

async function main (): Promise<boolean> {
  const opts = parseCommandLine();
  const outDir = path.resolve(opts['out-dir']);

  if (!fs.existsSync(outDir)) {
    throw new Error("Output directory doesn't exist");
  } else {
    // 确保COMPILE_COMMANDS.json文件是最新的
    const env = getDepotToolsEnv();

    const result = childProcess.spawnSync(
      'gn',
      ['gen', '.', '--export-compile-commands'],
      { cwd: outDir, env, shell: true }
    );

    if (result.status !== 0) {
      if (result.error) {
        console.error(result.error.message);
      } else {
        console.error(result.stderr.toString());
      }

      throw new ErrorWithExitCode(
        'Failed to automatically generate compile_commands.json for ' +
          'output directory',
        2
      );
    }
  }

  const filenames = [];

  if (opts._.length > 0) {
    filenames.push(...opts._.map((filename) => path.resolve(filename)));
  } else {
    filenames.push(
      ...(await findMatchingFiles(
        path.resolve(SOURCE_ROOT, 'shell'),
        (filename: string) => /.*\.(?:cc|h|mm)$/.test(filename)
      ))
    );
  }

  return runClangTidy(outDir, filenames, opts.checks, opts.jobs);
}

if (require.main === module) {
  main()
    .then((success) => {
      process.exit(success ? 0 : 1);
    })
    .catch((err: ErrorWithExitCode) => {
      console.error(`ERROR: ${err.message}`);
      process.exit(err.exitCode || 1);
    });
}
