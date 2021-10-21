#!/usr/bin/env node

const { version } = require('./package');

const childProcess = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');
const extract = require('extract-zip');
const { downloadArtifact } = require('@electron/get');

if (process.env.ELECTRON_SKIP_BINARY_DOWNLOAD) {
  process.exit(0);
}

const platformPath = getPlatformPath();

if (isInstalled()) {
  process.exit(0);
}

const platform = process.env.npm_config_platform || process.platform;
let arch = process.env.npm_config_arch || process.arch;

if (platform === 'darwin' && process.platform === 'darwin' && arch === 'x64') {
  // 在MacOS上下载MacOS时，我们认为我们需要x64，我们应该。
  // 检查我们是否在Rosetta下运行，如果合适，请下载arm64版本。
  try {
    const output = childProcess.execSync('sysctl -in sysctl.proc_translated');
    if (output.toString().trim() === '1') {
      arch = 'arm64';
    }
  } catch {
    // 忽略故障。
  }
}

// 下载(如果未缓存)。
downloadArtifact({
  version,
  artifactName: 'electron',
  force: process.env.force_no_cache === 'true',
  cacheRoot: process.env.electron_config_cache,
  checksums: process.env.electron_use_remote_checksums ? undefined : require('./checksums.json'),
  platform,
  arch
}).then(extractFile).catch(err => {
  console.error(err.stack);
  process.exit(1);
});

function isInstalled () {
  try {
    if (fs.readFileSync(path.join(__dirname, 'dist', 'version'), 'utf-8').replace(/^v/, '') !== version) {
      return false;
    }

    if (fs.readFileSync(path.join(__dirname, 'path.txt'), 'utf-8') !== platformPath) {
      return false;
    }
  } catch (ignored) {
    return false;
  }

  const electronPath = process.env.ELECTRON_OVERRIDE_DIST_PATH || path.join(__dirname, 'dist', platformPath);

  return fs.existsSync(electronPath);
}

// 解压缩并使path.txt指向正确的可执行文件
function extractFile (zipPath) {
  return new Promise((resolve, reject) => {
    extract(zipPath, { dir: path.join(__dirname, 'dist') }, err => {
      if (err) return reject(err);

      fs.writeFile(path.join(__dirname, 'path.txt'), platformPath, err => {
        if (err) return reject(err);

        resolve();
      });
    });
  });
}

function getPlatformPath () {
  const platform = process.env.npm_config_platform || os.platform();

  switch (platform) {
    case 'mas':
    case 'darwin':
      return 'Electron.app/Contents/MacOS/Electron';
    case 'freebsd':
    case 'openbsd':
    case 'linux':
      return 'electron';
    case 'win32':
      return 'electron.exe';
    default:
      throw new Error('Electron builds are not available on platform: ' + platform);
  }
}
