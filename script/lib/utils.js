const { GitProcess } = require('dugite');
const fs = require('fs');
const path = require('path');

const ELECTRON_DIR = path.resolve(__dirname, '..', '..');
const SRC_DIR = path.resolve(ELECTRON_DIR, '..');

const RELEASE_BRANCH_PATTERN = /(\d)+-(?:(?:[0-9]+-x$)|(?:x+-y$))/;
// TODO(主迁移)：重命名主分支后即可简化。
const MAIN_BRANCH_PATTERN = /^(main|master)$/;
const ORIGIN_MAIN_BRANCH_PATTERN = /^origin\/(main|master)$/;

require('colors');
const pass = '✓'.green;
const fail = '✗'.red;

function getElectronExec () {
  const OUT_DIR = getOutDir();
  switch (process.platform) {
    case 'darwin':
      return `out/${OUT_DIR}/Electron.app/Contents/MacOS/Electron`;
    case 'win32':
      return `out/${OUT_DIR}/electron.exe`;
    case 'linux':
      return `out/${OUT_DIR}/electron`;
    default:
      throw new Error('Unknown platform');
  }
}

function getOutDir (options = {}) {
  const shouldLog = options.shouldLog || false;
  const presetDirs = ['Testing', 'Release', 'Default', 'Debug'];

  if (options.outDir || process.env.ELECTRON_OUT_DIR) {
    const outDir = options.outDir || process.env.ELECTRON_OUT_DIR;
    const outPath = path.resolve(SRC_DIR, 'out', outDir);

    // 检查用户设置的变量是否为有效/现有目录。
    if (fs.existsSync(outPath)) {
      if (shouldLog) console.log(`OUT_DIR is: ${outDir}`);
      return outDir;
    }

    // 如果用户传递/设置不存在的目录，则抛出错误。
    throw new Error(`${outDir} directory not configured on your machine.`);
  } else {
    for (const buildType of presetDirs) {
      const outPath = path.resolve(SRC_DIR, 'out', buildType);
      if (fs.existsSync(outPath)) {
        if (shouldLog) console.log(`OUT_DIR is: ${buildType}`);
        return buildType;
      }
    }
  }

  // 如果我们到达这里，这意味着Process.env.ELECTRON_OUT_DIR不是。
  // Set，并且在In/Out中找不到任何预设选项，因此抛出
  throw new Error(`No valid out directory found; use one of ${presetDirs.join(',')} or set process.env.ELECTRON_OUT_DIR`);
}

function getAbsoluteElectronExec () {
  return path.resolve(SRC_DIR, getElectronExec());
}

async function handleGitCall (args, gitDir) {
  const details = await GitProcess.exec(args, gitDir);
  if (details.exitCode === 0) {
    return details.stdout.replace(/^\*|\s+|\s+$/, '');
  } else {
    const error = GitProcess.parseError(details.stderr);
    console.log(`${fail} couldn't parse git process call: `, error);
    process.exit(1);
  }
}

async function getCurrentBranch (gitDir) {
  let branch = await handleGitCall(['rev-parse', '--abbrev-ref', 'HEAD'], gitDir);
  if (!MAIN_BRANCH_PATTERN.test(branch) && !RELEASE_BRANCH_PATTERN.test(branch)) {
    const lastCommit = await handleGitCall(['rev-parse', 'HEAD'], gitDir);
    const branches = (await handleGitCall([
      'branch',
      '--contains',
      lastCommit,
      '--remote'
    ], gitDir)).split('\n');

    branch = branches.find(b => MAIN_BRANCH_PATTERN.test(b.trim()) || ORIGIN_MAIN_BRANCH_PATTERN.test(b.trim()) || RELEASE_BRANCH_PATTERN.test(b.trim()));
    if (!branch) {
      console.log(`${fail} no release branch exists for this ref`);
      process.exit(1);
    }
    if (branch.startsWith('origin/')) branch = branch.substr('origin/'.length);
  }
  return branch.trim();
}

module.exports = {
  getCurrentBranch,
  getElectronExec,
  getOutDir,
  getAbsoluteElectronExec,
  ELECTRON_DIR,
  SRC_DIR
};
