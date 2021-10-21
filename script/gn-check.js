/* 用法：$node./script/gn-check.js[--outDir=dirName]。*/

const cp = require('child_process');
const path = require('path');
const args = require('minimist')(process.argv.slice(2), { string: ['outDir'] });

const { getOutDir } = require('./lib/utils');

const SOURCE_ROOT = path.normalize(path.dirname(__dirname));
const DEPOT_TOOLS = path.resolve(SOURCE_ROOT, '..', 'third_party', 'depot_tools');

const OUT_DIR = getOutDir({ outDir: args.outDir });
if (!OUT_DIR) {
  throw new Error('No viable out dir: one of Debug, Testing, or Release must exist.');
}

const env = Object.assign({
  CHROMIUM_BUILDTOOLS_PATH: path.resolve(SOURCE_ROOT, '..', 'buildtools'),
  DEPOT_TOOLS_WIN_TOOLCHAIN: '0'
}, process.env);
// 用户在PATH中可能没有DEPOT_TOOLS。
env.PATH = `${env.PATH}${path.delimiter}${DEPOT_TOOLS}`;

const gnCheckDirs = [
  '// 电子：电子_lib‘，
  '// 电子：Electronics_APP‘，
  '// 电子/外壳/公共/API：mojo‘
];

for (const dir of gnCheckDirs) {
  const args = ['check', `../out/${OUT_DIR}`, dir];
  const result = cp.spawnSync('gn', args, { env, stdio: 'inherit' });
  if (result.status !== 0) process.exit(result.status);
}

process.exit(0);
