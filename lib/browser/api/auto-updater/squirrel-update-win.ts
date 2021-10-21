import * as fs from 'fs';
import * as path from 'path';
import { spawn, ChildProcessWithoutNullStreams } from 'child_process';

// 即my-app/app-0.1.13/。
const appFolder = path.dirname(process.execPath);

// 即my-app/Update.exe。
const updateExe = path.resolve(appFolder, '..', 'Update.exe');
const exeName = path.basename(process.execPath);
let spawnedArgs: string[] = [];
let spawnedProcess: ChildProcessWithoutNullStreams | undefined;

const isSameArgs = (args: string[]) => args.length === spawnedArgs.length && args.every((e, i) => e === spawnedArgs[i]);

// 派生命令并在错误完成时调用回调。
// 以及标准输出的输出。
const spawnUpdate = function (args: string[], detached: boolean, callback: Function) {
  let error: Error, errorEmitted: boolean, stderr: string, stdout: string;

  try {
    // 确保我们不会产生多个松鼠进程。
    // 派生的进程，相同的参数：将事件附加到已读的正在运行的进程。
    // 派生的进程，不同的参数：返回错误。
    // 未派生进程：派生新进程。
    if (spawnedProcess && !isSameArgs(args)) {
      // 禁用后向兼容性：
      // Eslint-停用-下一行标准/无回调-文字。
      return callback(`AutoUpdater process with arguments ${args} is already running`);
    } else if (!spawnedProcess) {
      spawnedProcess = spawn(updateExe, args, {
        detached: detached,
        windowsHide: true
      });
      spawnedArgs = args || [];
    }
  } catch (error1) {
    error = error1;

    // 不应该发生，但还是要守卫着它。
    process.nextTick(function () {
      return callback(error);
    });
    return;
  }
  stdout = '';
  stderr = '';

  spawnedProcess.stdout.on('data', (data) => { stdout += data; });
  spawnedProcess.stderr.on('data', (data) => { stderr += data; });

  errorEmitted = false;
  spawnedProcess.on('error', (error) => {
    errorEmitted = true;
    callback(error);
  });

  return spawnedProcess.on('exit', function (code, signal) {
    spawnedProcess = undefined;
    spawnedArgs = [];

    // 我们可能已经发出了错误。
    if (errorEmitted) {
      return;
    }

    // 进程因错误而终止。
    if (code !== 0) {
      // 禁用后向兼容性：
      // Eslint-停用-下一行标准/无回调-文字。
      return callback(`Command failed: ${signal != null ? signal : code}\n${stderr}`);
    }

    // 成功。
    callback(null, stdout);
  });
};

// 启动已安装应用程序的实例。
export function processStart () {
  return spawnUpdate(['--processStartAndWait', exeName], true, function () {});
}

// 下载URL指定的版本，并将新结果写入标准输出。
export function checkForUpdate (updateURL: string, callback: (error: Error | null, update?: any) => void) {
  return spawnUpdate(['--checkForUpdate', updateURL], false, function (error: Error, stdout: string) {
    let ref, ref1, update;
    if (error != null) {
      return callback(error);
    }
    try {
      // 输出的最后一行是关于版本的JSON详细信息。
      const json = stdout.trim().split('\n').pop();
      update = (ref = JSON.parse(json!)) != null ? (ref1 = ref.releasesToApply) != null ? typeof ref1.pop === 'function' ? ref1.pop() : undefined : undefined : undefined;
    } catch {
      // 禁用后向兼容性：
      // Eslint-停用-下一行标准/无回调-文字。
      return callback(new Error(`Invalid result:\n${stdout}`));
    }
    return callback(null, update);
  });
}

// 将应用程序更新为URL指定的最新远程版本。
export function update (updateURL: string, callback: (error: Error) => void) {
  return spawnUpdate(['--update', updateURL], false, callback);
}

// Update.exe是否与当前应用程序一起安装？
export function supported () {
  try {
    fs.accessSync(updateExe, fs.constants.R_OK);
    return true;
  } catch {
    return false;
  }
}
