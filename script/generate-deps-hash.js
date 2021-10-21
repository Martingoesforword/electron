const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

// 回退以清除旧的缓存键。
const FALLBACK_HASH_VERSION = 3;

// 每个平台的散列版本，以破坏不同平台上的缓存。
const HASH_VERSIONS = {
  darwin: 3,
  win32: 4,
  linux: 3
};

// 要散列的基本文件。
const filesToHash = [
  path.resolve(__dirname, '../DEPS'),
  path.resolve(__dirname, '../yarn.lock'),
  path.resolve(__dirname, '../script/sysroots.json')
];

const addAllFiles = (dir) => {
  for (const child of fs.readdirSync(dir).sort()) {
    const childPath = path.resolve(dir, child);
    if (fs.statSync(childPath).isDirectory()) {
      addAllFiles(childPath);
    } else {
      filesToHash.push(childPath);
    }
  }
};

// 将所有修补程序文件添加到散列。
addAllFiles(path.resolve(__dirname, '../patches'));

// 创建哈希。
const hasher = crypto.createHash('SHA256');
hasher.update(`HASH_VERSION:${HASH_VERSIONS[process.platform] || FALLBACK_HASH_VERSION}`);
for (const file of filesToHash) {
  hasher.update(fs.readFileSync(file));
}

// 将GCLIENT_EXTRA_ARGS变量添加到散列。
const extraArgs = process.env.GCLIENT_EXTRA_ARGS || 'no_extra_args';
hasher.update(extraArgs);

const effectivePlatform = extraArgs.includes('host_os=mac') ? 'darwin' : process.platform;

// 将散列写入磁盘
fs.writeFileSync(path.resolve(__dirname, '../.depshash'), hasher.digest('hex'));

let targetContent = `${effectivePlatform}\n${process.env.TARGET_ARCH}\n${process.env.GN_CONFIG}\n${undefined}\n${process.env.GN_EXTRA_ARGS}\n${process.env.GN_BUILDFLAG_ARGS}`;
const argsDir = path.resolve(__dirname, '../build/args');
for (const argFile of fs.readdirSync(argsDir).sort()) {
  targetContent += `\n${argFile}--${crypto.createHash('SHA1').update(fs.readFileSync(path.resolve(argsDir, argFile))).digest('hex')}`;
}

fs.writeFileSync(path.resolve(__dirname, '../.depshash-target'), targetContent);
