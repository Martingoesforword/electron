#!/usr/bin/env node

const { GitProcess } = require('dugite');
const { promises: fs } = require('fs');
const semver = require('semver');
const path = require('path');
const minimist = require('minimist');

const { ELECTRON_DIR } = require('../lib/utils');
const versionUtils = require('./version-utils');
const supported = path.resolve(ELECTRON_DIR, 'docs', 'tutorial', 'support.md');

const writeFile = fs.writeFile;
const readFile = fs.readFile;

function parseCommandLine () {
  let help;
  const opts = minimist(process.argv.slice(2), {
    string: ['bump', 'version'],
    boolean: ['dryRun', 'help'],
    alias: { version: ['v'] },
    unknown: arg => { help = true; }
  });
  if (help || opts.help || !opts.bump) {
    console.log(`
      Bump release version number. Possible arguments:\n
        --bump=patch to increment patch version\n
        --version={version} to set version number directly\n
        --dryRun to print the next version without updating files
      Note that you can use both --bump and --stable  simultaneously. 
    `);
    process.exit(0);
  }
  return opts;
}

// 运行脚本。
async function main () {
  const opts = parseCommandLine();
  const currentVersion = await versionUtils.getElectronVersion();
  const version = await nextVersion(opts.bump, currentVersion);

  const parsed = semver.parse(version);
  const components = {
    major: parsed.major,
    minor: parsed.minor,
    patch: parsed.patch,
    pre: parsed.prerelease
  };

  // 打印潜在的新版本并提前退出。
  if (opts.dryRun) {
    console.log(`new version number would be: ${version}\n`);
    return 0;
  }

  if (shouldUpdateSupported(opts.bump, currentVersion, version)) {
    await updateSupported(version, supported);
  }

  // 更新所有与版本相关的文件。
  await Promise.all([
    updateVersion(version),
    updatePackageJSON(version),
    updateWinRC(components)
  ]);

  // 提交所有更新的版本相关文件。
  await commitVersionBump(version);

  console.log(`Bumped to version: ${version}`);
}

// 根据[每夜、阿尔法、测试版、稳定版]获取下一版本的发行版。
async function nextVersion (bumpType, version) {
  if (
    versionUtils.isNightly(version) ||
    versionUtils.isAlpha(version) ||
    versionUtils.isBeta(version)
  ) {
    switch (bumpType) {
      case 'nightly':
        version = await versionUtils.nextNightly(version);
        break;
      case 'alpha':
        version = await versionUtils.nextAlpha(version);
        break;
      case 'beta':
        version = await versionUtils.nextBeta(version);
        break;
      case 'stable':
        version = semver.valid(semver.coerce(version));
        break;
      default:
        throw new Error('Invalid bump type.');
    }
  } else if (versionUtils.isStable(version)) {
    switch (bumpType) {
      case 'nightly':
        version = versionUtils.nextNightly(version);
        break;
      case 'alpha':
        throw new Error('Cannot bump to alpha from stable.');
      case 'beta':
        throw new Error('Cannot bump to beta from stable.');
      case 'minor':
        version = semver.inc(version, 'minor');
        break;
      case 'stable':
        version = semver.inc(version, 'patch');
        break;
      default:
        throw new Error('Invalid bump type.');
    }
  } else {
    throw new Error(`Invalid current version: ${version}`);
  }
  return version;
}

function shouldUpdateSupported (bump, current, version) {
  return isMajorStable(bump, current) || isMajorNightly(version, current);
}

function isMajorStable (bump, currentVersion) {
  if (versionUtils.isBeta(currentVersion) && (bump === 'stable')) return true;
  return false;
}

function isMajorNightly (version, currentVersion) {
  const parsed = semver.parse(version);
  const current = semver.parse(currentVersion);
  if (versionUtils.isNightly(currentVersion) && (parsed.major > current.major)) return true;
  return false;
}

// 使用最新版本信息更新版本文件。
async function updateVersion (version) {
  const versionPath = path.resolve(ELECTRON_DIR, 'ELECTRON_VERSION');
  await writeFile(versionPath, version, 'utf8');
}

// 使用新版本更新包元数据文件。
async function updatePackageJSON (version) {
  const filePath = path.resolve(ELECTRON_DIR, 'package.json');
  const file = require(filePath);
  file.version = version;
  await writeFile(filePath, JSON.stringify(file, null, 2));
}

// 将凹凸提交推送到释放分支。
async function commitVersionBump (version) {
  const gitArgs = ['commit', '-a', '-m', `Bump v${version}`, '-n'];
  await GitProcess.exec(gitArgs, ELECTRON_DIR);
}

// 使用新的Semver值更新Electron.rc文件。
async function updateWinRC (components) {
  const filePath = path.resolve(ELECTRON_DIR, 'shell', 'browser', 'resources', 'win', 'electron.rc');
  const data = await readFile(filePath, 'utf8');
  const arr = data.split('\n');
  arr.forEach((line, idx) => {
    if (line.includes('FILEVERSION')) {
      arr[idx] = ` FILEVERSION ${versionUtils.makeVersion(components, ',', versionUtils.preType.PARTIAL)}`;
      arr[idx + 1] = ` PRODUCTVERSION ${versionUtils.makeVersion(components, ',', versionUtils.preType.PARTIAL)}`;
    } else if (line.includes('FileVersion')) {
      arr[idx] = `            VALUE "FileVersion", "${versionUtils.makeVersion(components, '.')}"`;
      arr[idx + 5] = `            VALUE "ProductVersion", "${versionUtils.makeVersion(components, '.')}"`;
    }
  });
  await writeFile(filePath, arr.join('\n'));
}

// 使用新的Semver值更新support.md文件(仅限稳定)
async function updateSupported (version, filePath) {
  const v = parseInt(version);
  const newVersions = [`* ${v}.x.y`, `* ${v - 1}.x.y`, `* ${v - 2}.x.y`, `* ${v - 3}`];
  const contents = await readFile(filePath, 'utf8');
  const previousVersions = contents.split('\n').filter((elem) => {
    return (/[^\n]*\.x\.y[^\n]*/).test(elem);
  }, []);

  const newContents = previousVersions.reduce((contents, current, i) => {
    return contents.replace(current, newVersions[i]);
  }, contents);

  await writeFile(filePath, newContents, 'utf8');
}

if (process.mainModule === module) {
  main().catch((error) => {
    console.error(error);
    process.exit(1);
  });
}

module.exports = { nextVersion, shouldUpdateSupported, updateSupported };
