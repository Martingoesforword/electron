import { spawnSync } from 'child_process';
import * as path from 'path';

const srcPath = path.resolve(__dirname, '..', '..', '..');
const patchExportFnPath = path.resolve(__dirname, 'export_all_patches.py');
const configPath = path.resolve(__dirname, '..', 'patches', 'config.json');

// 重新导出所有补丁程序以检查是否有更改。
const proc = spawnSync('python', [patchExportFnPath, configPath, '--dry-run'], {
  cwd: srcPath
});

// 如果补丁导出返回1，则失败，例如试运行失败。
if (proc.status === 1) {
  console.log(proc.stderr.toString('utf8'));
  process.exit(1);
}
