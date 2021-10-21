const temp = require('temp');
const fs = require('fs');
const path = require('path');
const childProcess = require('child_process');
const got = require('got');
const semver = require('semver');

const { getCurrentBranch, ELECTRON_DIR } = require('../lib/utils');
const rootPackageJson = require('../../package.json');

const { Octokit } = require('@octokit/rest');
const { getAssetContents } = require('./get-asset');
const octokit = new Octokit({
  userAgent: 'electron-npm-publisher',
  auth: process.env.ELECTRON_GITHUB_TOKEN
});

if (!process.env.ELECTRON_NPM_OTP) {
  console.error('Please set ELECTRON_NPM_OTP');
  process.exit(1);
}

let tempDir;
temp.track(); // 在退出时跟踪和清理文件。

const files = [
  'cli.js',
  'index.js',
  'install.js',
  'package.json',
  'README.md',
  'LICENSE'
];

const jsonFields = [
  'name',
  'version',
  'repository',
  'description',
  'license',
  'author',
  'keywords'
];

let npmTag = '';

new Promise((resolve, reject) => {
  temp.mkdir('electron-npm', (err, dirPath) => {
    if (err) {
      reject(err);
    } else {
      resolve(dirPath);
    }
  });
})
  .then((dirPath) => {
    tempDir = dirPath;
    // 将文件从`/npm`复制到临时目录。
    files.forEach((name) => {
      const noThirdSegment = name === 'README.md' || name === 'LICENSE';
      fs.writeFileSync(
        path.join(tempDir, name),
        fs.readFileSync(path.join(ELECTRON_DIR, noThirdSegment ? '' : 'npm', name))
      );
    });
    // 从根Package.json复制到temp/Package.json。
    const packageJson = require(path.join(tempDir, 'package.json'));
    jsonFields.forEach((fieldName) => {
      packageJson[fieldName] = rootPackageJson[fieldName];
    });
    fs.writeFileSync(
      path.join(tempDir, 'package.json'),
      JSON.stringify(packageJson, null, 2)
    );

    return octokit.repos.listReleases({
      owner: 'electron',
      repo: rootPackageJson.version.indexOf('nightly') > 0 ? 'nightlies' : 'electron'
    });
  })
  .then((releases) => {
  // 从Release下载Electron.d.ts。
    const release = releases.data.find(
      (release) => release.tag_name === `v${rootPackageJson.version}`
    );
    if (!release) {
      throw new Error(`cannot find release with tag v${rootPackageJson.version}`);
    }
    return release;
  })
  .then(async (release) => {
    const tsdAsset = release.assets.find((asset) => asset.name === 'electron.d.ts');
    if (!tsdAsset) {
      throw new Error(`cannot find electron.d.ts from v${rootPackageJson.version} release assets`);
    }

    const typingsContent = await getAssetContents(
      rootPackageJson.version.indexOf('nightly') > 0 ? 'nightlies' : 'electron',
      tsdAsset.id
    );

    fs.writeFileSync(path.join(tempDir, 'electron.d.ts'), typingsContent);

    return release;
  })
  .then(async (release) => {
    const checksumsAsset = release.assets.find((asset) => asset.name === 'SHASUMS256.txt');
    if (!checksumsAsset) {
      throw new Error(`cannot find SHASUMS256.txt from v${rootPackageJson.version} release assets`);
    }

    const checksumsContent = await getAssetContents(
      rootPackageJson.version.indexOf('nightly') > 0 ? 'nightlies' : 'electron',
      checksumsAsset.id
    );

    const checksumsObject = {};
    for (const line of checksumsContent.trim().split('\n')) {
      const [checksum, file] = line.split(' *');
      checksumsObject[file] = checksum;
    }

    fs.writeFileSync(path.join(tempDir, 'checksums.json'), JSON.stringify(checksumsObject, null, 2));

    return release;
  })
  .then(async (release) => {
    const currentBranch = await getCurrentBranch();

    if (release.tag_name.indexOf('nightly') > 0) {
      // TODO(主迁移)：重命名主分支后进行简化。
      if (currentBranch === 'master' || currentBranch === 'main') {
        // 夜行者被发布到他们自己的模块，所以他们应该被标记为最新的。
        npmTag = 'latest';
      } else {
        npmTag = `nightly-${currentBranch}`;
      }

      const currentJson = JSON.parse(fs.readFileSync(path.join(tempDir, 'package.json'), 'utf8'));
      currentJson.name = 'electron-nightly';
      rootPackageJson.name = 'electron-nightly';

      fs.writeFileSync(
        path.join(tempDir, 'package.json'),
        JSON.stringify(currentJson, null, 2)
      );
    } else {
      if (currentBranch === 'master' || currentBranch === 'main') {
        // 这种情况永远不会发生，主要版本应该是每晚发布。
        // 这是以防万一的。
        throw new Error('Unreachable release phase, can\'t tag a non-nightly release on the main branch');
      } else if (!release.prerelease) {
        // 使用`2-0-x`样式标记标记版本。
        npmTag = currentBranch;
      } else if (release.tag_name.indexOf('alpha') > 0) {
        // 使用`alpha-3-0-x`样式标记标记版本。
        npmTag = `alpha-${currentBranch}`;
      } else {
        // 使用`beta-3-0-x`样式标记为发行版添加标签。
        npmTag = `beta-${currentBranch}`;
      }
    }
  })
  .then(() => childProcess.execSync('npm pack', { cwd: tempDir }))
  .then(() => {
  // 测试该软件包可以安装从GitHub版本预置的电子。
    const tarballPath = path.join(tempDir, `${rootPackageJson.name}-${rootPackageJson.version}.tgz`);
    return new Promise((resolve, reject) => {
      const result = childProcess.spawnSync('npm', ['install', tarballPath, '--force', '--silent'], {
        env: Object.assign({}, process.env, { electron_config_cache: tempDir }),
        cwd: tempDir,
        stdio: 'inherit'
      });
      if (result.status !== 0) {
        return reject(new Error(`npm install failed with status ${result.status}`));
      }
      try {
        const electronPath = require(path.resolve(tempDir, 'node_modules', rootPackageJson.name));
        if (typeof electronPath !== 'string') {
          return reject(new Error(`path to electron binary (${electronPath}) returned by the ${rootPackageJson.name} module is not a string`));
        }
        if (!fs.existsSync(electronPath)) {
          return reject(new Error(`path to electron binary (${electronPath}) returned by the ${rootPackageJson.name} module does not exist on disk`));
        }
      } catch (e) {
        console.error(e);
        return reject(new Error(`loading the generated ${rootPackageJson.name} module failed with an error`));
      }
      resolve(tarballPath);
    });
  })
  .then((tarballPath) => {
    const existingVersionJSON = childProcess.execSync(`npm view electron@${rootPackageJson.version} --json`).toString('utf-8');
    // 有可能这是重新运行，我们已经发布了包，如果没有，我们只是像正常一样发布。
    if (!existingVersionJSON) {
      childProcess.execSync(`npm publish ${tarballPath} --tag ${npmTag} --otp=${process.env.ELECTRON_NPM_OTP}`);
    }
  })
  .then(() => {
    const currentTags = JSON.parse(childProcess.execSync('npm show electron dist-tags --json').toString());
    const localVersion = rootPackageJson.version;
    const parsedLocalVersion = semver.parse(localVersion);
    if (rootPackageJson.name === 'electron') {
      // 我们应该只为包名称仍然存在的非夜间版本自定义添加dist标记。
      // “电子”
      if (parsedLocalVersion.prerelease.length === 0 &&
            semver.gt(localVersion, currentTags.latest)) {
        childProcess.execSync(`npm dist-tag add electron@${localVersion} latest --otp=${process.env.ELECTRON_NPM_OTP}`);
      }
      if (parsedLocalVersion.prerelease[0] === 'beta' &&
            semver.gt(localVersion, currentTags.beta)) {
        childProcess.execSync(`npm dist-tag add electron@${localVersion} beta --otp=${process.env.ELECTRON_NPM_OTP}`);
      }
      if (parsedLocalVersion.prerelease[0] === 'alpha' &&
            semver.gt(localVersion, currentTags.alpha)) {
        childProcess.execSync(`npm dist-tag add electron@${localVersion} alpha --otp=${process.env.ELECTRON_NPM_OTP}`);
      }
    }
  })
  .catch((err) => {
    console.error('Error:', err);
    process.exit(1);
  });
