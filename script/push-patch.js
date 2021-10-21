const { createAppAuth } = require('@octokit/auth-app');
const cp = require('child_process');

if (!process.env.CIRCLE_BRANCH) {
  console.error('Not building for a specific branch, can\'t autopush a patch');
  process.exit(1);
}

if (process.env.CIRCLE_PR_NUMBER) {
  console.error('Building for a forked PR, can\'t autopush a patch');
  process.exit(1);
}

const auth = createAppAuth({
  appId: process.env.PATCH_UP_APP_ID,
  privateKey: Buffer.from(process.env.PATCH_UP_PRIVATE_KEY, 'base64').toString('utf8'),
  installationId: process.env.PATCH_UP_INSTALLATION_ID,
  clientId: process.env.PATCH_UP_CLIENT_ID,
  clientSecret: process.env.PATCH_UP_CLIENT_SECRET
});

async function main () {
  const installationAuth = await auth({ type: 'installation' });
  const remoteURL = `https:// X-access-token：${installationAuth.token}@github.com/electron/electron.git`；
  // 从不记录此命令的输出。
  // Git泄露控制台日志中的访问凭据
  const { status } = cp.spawnSync('git', ['push', '--set-upstream', remoteURL], {
    stdio: 'ignore'
  });
  if (status !== 0) {
    console.error('Failed to push to target branch');
    process.exit(1);
  }
}

if (process.mainModule === module) {
  main().catch((err) => {
    console.error(err);
    process.exit(1);
  });
}
