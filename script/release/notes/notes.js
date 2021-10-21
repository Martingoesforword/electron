#!/usr/bin/env node

'use strict';

const fs = require('fs');
const path = require('path');

const { GitProcess } = require('dugite');

const { Octokit } = require('@octokit/rest');
const octokit = new Octokit({
  auth: process.env.ELECTRON_GITHUB_TOKEN
});

const { ELECTRON_DIR } = require('../../lib/utils');

const MAX_FAIL_COUNT = 3;
const CHECK_INTERVAL = 5000;

const TROP_LOGIN = 'trop[bot]';

const NO_NOTES = 'No notes';

const docTypes = new Set(['doc', 'docs']);
const featTypes = new Set(['feat', 'feature']);
const fixTypes = new Set(['fix']);
const otherTypes = new Set(['spec', 'build', 'test', 'chore', 'deps', 'refactor', 'tools', 'perf', 'style', 'ci']);
const knownTypes = new Set([...docTypes.keys(), ...featTypes.keys(), ...fixTypes.keys(), ...otherTypes.keys()]);

const getCacheDir = () => process.env.NOTES_CACHE_PATH || path.resolve(__dirname, '.cache');

/* *。*/

// 指向GitHub项目的链接，例如问题或拉入请求。
class GHKey {
  constructor (owner, repo, number) {
    this.owner = owner;
    this.repo = repo;
    this.number = number;
  }

  static NewFromPull (pull) {
    const owner = pull.base.repo.owner.login;
    const repo = pull.base.repo.name;
    const number = pull.number;
    return new GHKey(owner, repo, number);
  }
}

class Commit {
  constructor (hash, owner, repo) {
    this.hash = hash; // 细绳。
    this.owner = owner; // 细绳。
    this.repo = repo; // 细绳。

    this.isBreakingChange = false;
    this.note = null; // 细绳。

    // 此更改已合并到的一组分支。
    // ‘8-x-y’=&gt;GHKey{所有者：‘电子’，回购：‘电子’，编号：23714}。
    this.trops = new Map(); // MAP&lt;字符串，GHKey&gt;。

    this.prKeys = new Set(); // GHKey。
    this.revertHash = null; // 细绳。
    this.semanticType = null; // 细绳。
    this.subject = null; // 细绳。
  }
}

class Pool {
  constructor () {
    this.commits = []; // 数组&lt;Commit&gt;。
    this.processedHashes = new Set();
    this.pulls = {}; // GHKey.number=&gt;octokit拉取对象。
  }
}

/* *。*/

const runGit = async (dir, args) => {
  const response = await GitProcess.exec(args, dir);
  if (response.exitCode !== 0) {
    throw new Error(response.stderr.trim());
  }
  return response.stdout.trim();
};

const getCommonAncestor = async (dir, point1, point2) => {
  return runGit(dir, ['merge-base', point1, point2]);
};

const getNoteFromClerk = async (ghKey) => {
  const comments = await getComments(ghKey);
  if (!comments || !comments.data) return;

  const CLERK_LOGIN = 'release-clerk[bot]';
  const CLERK_NO_NOTES = '**No Release Notes**';
  const PERSIST_LEAD = '**Release Notes Persisted**\n\n';
  const QUOTE_LEAD = '> ';

  for (const comment of comments.data.reverse()) {
    if (comment.user.login !== CLERK_LOGIN) {
      continue;
    }
    if (comment.body === CLERK_NO_NOTES) {
      return NO_NOTES;
    }
    if (comment.body.startsWith(PERSIST_LEAD)) {
      let lines = comment.body
        .slice(PERSIST_LEAD.length).trim() // 删除Persistent_Lead。
        .split(/\r?\n/) // 拆分成行。
        .map(line => line.trim())
        .filter(line => line.startsWith(QUOTE_LEAD)) // 注释被引用。
        .map(line => line.slice(QUOTE_LEAD.length)); // 对这些行加引号。

      const firstLine = lines.shift();
      // 缩进第一行之后的任何内容以确保。
      // 具有自己的子列表的多行注释不会。
      // 在标记中解析为顶级列表的一部分。
      // (示例：https://github.com/electron/electron/pull/25216)。
      lines = lines.map(line => '  ' + line);
      return [firstLine, ...lines]
        .join('\n') // 加入队伍。
        .trim();
    }
  }
};

/* **在提交消息中查找我们项目的约定：**‘语义：一些描述’--设置语义类型，主题*‘一些描述(#99999)’--设置主题，Pr*‘从${BranchName}合并拉取请求#99999’--设置pr*‘This Reverts Commit${sha}’--设置revertHash*Body中以‘Breaking Change’开头的行--sets isBreakingChange*‘Backport of#99999’--设置pr。*/
const parseCommitMessage = (commitMessage, commit) => {
  const { owner, repo } = commit;

  // 将提交消息拆分为主题和正文。
  let subject = commitMessage;
  let body = '';
  const pos = subject.indexOf('\n');
  if (pos !== -1) {
    body = subject.slice(pos).trim();
    subject = subject.slice(0, pos).trim();
  }

  // 如果主题以‘(#dddd)’结尾，则将其视为拉取请求id。
  let match;
  if ((match = subject.match(/^(.*)\s\(#(\d+)\)$/))) {
    commit.prKeys.add(new GHKey(owner, repo, parseInt(match[2])));
    subject = match[1];
  }

  // 如果主语以‘word：’开头，则将其视为语义提交。
  if ((match = subject.match(/^(\w+):\s(.*)$/))) {
    const semanticType = match[1].toLocaleLowerCase();
    if (knownTypes.has(semanticType)) {
      commit.semanticType = semanticType;
      subject = match[2];
    }
  }

  // 检查指示PR的GitHub提交消息。
  if ((match = subject.match(/^Merge pull request #(\d+) from (.*)$/))) {
    commit.prKeys.add(new GHKey(owner, repo, parseInt(match[1])));
  }

  // 检查是否有指示PR的注释。
  const backportPattern = /(?:^|\n)(?:manual |manually )?backport.*(?:#(\d+)|\/pull\/(\d+))/im;
  if ((match = commitMessage.match(backportPattern))) {
    // 这可能是第一个或第二个捕获组，具体取决于它是否是链接。
    const backportNumber = match[1] ? parseInt(match[1], 10) : parseInt(match[2], 10);
    commit.prKeys.add(new GHKey(owner, repo, backportNumber));
  }

  // Https://help.github.com/articles/closing-issues-using-keywords/。
  if (body.match(/\b(?:close|closes|closed|fix|fixes|fixed|resolve|resolves|resolved|for)\s#(\d+)\b/i)) {
    commit.semanticType = commit.semanticType || 'fix';
  }

  // Https://www.conventionalcommits.org/en。
  if (commitMessage
    .split(/\r?\n/) // 拆分成行。
    .map(line => line.trim())
    .some(line => line.startsWith('BREAKING CHANGE'))) {
    commit.isBreakingChange = true;
  }

  // 检查是否有返回提交。
  if ((match = body.match(/This reverts commit ([a-f0-9]{40})\./))) {
    commit.revertHash = match[1];
  }

  commit.subject = subject.trim();
  return commit;
};

const parsePullText = (pull, commit) => parseCommitMessage(`${pull.data.title}\n\n${pull.data.body}`, commit);

const getLocalCommitHashes = async (dir, ref) => {
  const args = ['log', '--format=%H', ref];
  return (await runGit(dir, args))
    .split(/\r?\n/) // 拆分成行。
    .map(hash => hash.trim());
};

// 返回提交数组。
const getLocalCommits = async (module, point1, point2) => {
  const { owner, repo, dir } = module;

  const fieldSep = ',';
  const format = ['%H', '%s'].join(fieldSep);
  const args = ['log', '--cherry-pick', '--right-only', '--first-parent', `--format=${format}`, `${point1}..${point2}`];
  const logs = (await runGit(dir, args))
    .split(/\r?\n/) // 拆分成行。
    .map(field => field.trim());

  const commits = [];
  for (const log of logs) {
    if (!log) {
      continue;
    }
    const [hash, subject] = log.split(fieldSep, 2).map(field => field.trim());
    commits.push(parseCommitMessage(subject, new Commit(hash, owner, repo)));
  }
  return commits;
};

const checkCache = async (name, operation) => {
  const filename = path.resolve(getCacheDir(), name);
  if (fs.existsSync(filename)) {
    return JSON.parse(fs.readFileSync(filename, 'utf8'));
  }
  process.stdout.write('.');
  const response = await operation();
  if (response) {
    fs.writeFileSync(filename, JSON.stringify(response));
  }
  return response;
};

// Helper函数，可为易失性GH API端点增加一定的弹性。
async function runRetryable (fn, maxRetries) {
  let lastError;
  for (let i = 0; i < maxRetries; i++) {
    try {
      return await fn();
    } catch (error) {
      await new Promise((resolve, reject) => setTimeout(resolve, CHECK_INTERVAL));
      lastError = error;
    }
  }
  // 默默地吃404。
  // 默默地吃着422，它们来自“没有找到SHA的提交”(No Commit Found For SHA)。
  if (lastError.status !== 404 && lastError.status !== 422) throw lastError;
}

const getPullCacheFilename = ghKey => `${ghKey.owner}-${ghKey.repo}-pull-${ghKey.number}`;

const getCommitPulls = async (owner, repo, hash) => {
  const name = `${owner}-${repo}-commit-${hash}`;
  const retryableFunc = () => octokit.repos.listPullRequestsAssociatedWithCommit({ owner, repo, commit_sha: hash });
  let ret = await checkCache(name, () => runRetryable(retryableFunc, MAX_FAIL_COUNT));

  // 只有合并的拉动才属于发行说明。
  if (ret && ret.data) {
    ret.data = ret.data.filter(pull => pull.merged_at);
  }

  // 缓存拉入。
  if (ret && ret.data) {
    for (const pull of ret.data) {
      const cachefile = getPullCacheFilename(GHKey.NewFromPull(pull));
      const payload = { ...ret, data: pull };
      await checkCache(cachefile, () => payload);
    }
  }

  // 确保返回值具有预期的结构，即使在失败的情况下也是如此。
  if (!ret || !ret.data) {
    ret = { data: [] };
  }

  return ret;
};

const getPullRequest = async (ghKey) => {
  const { number, owner, repo } = ghKey;
  const name = getPullCacheFilename(ghKey);
  const retryableFunc = () => octokit.pulls.get({ pull_number: number, owner, repo });
  return checkCache(name, () => runRetryable(retryableFunc, MAX_FAIL_COUNT));
};

const getComments = async (ghKey) => {
  const { number, owner, repo } = ghKey;
  const name = `${owner}-${repo}-issue-${number}-comments`;
  const retryableFunc = () => octokit.issues.listComments({ issue_number: number, owner, repo, per_page: 100 });
  return checkCache(name, () => runRetryable(retryableFunc, MAX_FAIL_COUNT));
};

const addRepoToPool = async (pool, repo, from, to) => {
  const commonAncestor = await getCommonAncestor(repo.dir, from, to);

  // 把老分行的承诺标记为旧新闻。
  for (const oldHash of await getLocalCommitHashes(repo.dir, from)) {
    pool.processedHashes.add(oldHash);
  }

  // 获取新分支的提交和与其相关联的拉取。
  const commits = await getLocalCommits(repo, commonAncestor, to);
  for (const commit of commits) {
    const { owner, repo, hash } = commit;
    for (const pull of (await getCommitPulls(owner, repo, hash)).data) {
      commit.prKeys.add(GHKey.NewFromPull(pull));
    }
  }

  pool.commits.push(...commits);

  // 加上拉力。
  for (const commit of commits) {
    let prKey;
    for (prKey of commit.prKeys.values()) {
      const pull = await getPullRequest(prKey);
      if (!pull || !pull.data) continue; // 我拿不到。
      pool.pulls[prKey.number] = pull;
      parsePullText(pull, commit);
    }
  }
};

// @Return Map&lt;String，GHKey&gt;。
// 其中键是分支名称(例如‘7-1-x’或‘8-x-y’)。
// 它的价值就是公关的GHKey。
async function getMergedTrops (commit, pool) {
  const branches = new Map();

  for (const prKey of commit.prKeys.values()) {
    const pull = pool.pulls[prKey.number];
    const mergedBranches = new Set(
      ((pull && pull.data && pull.data.labels) ? pull.data.labels : [])
        .map(label => ((label && label.name) ? label.name : '').match(/merged\/([0-9]+-[x0-9]-[xy0-9])/))
        .filter(match => match)
        .map(match => match[1])
    );

    if (mergedBranches.size > 0) {
      const isTropComment = (comment) => comment && comment.user && comment.user.login === TROP_LOGIN;

      const ghKey = GHKey.NewFromPull(pull.data);
      const backportRegex = /backported this PR to "(.*)",\s+please check out #(\d+)/;
      const getBranchNameAndPullKey = (comment) => {
        const match = ((comment && comment.body) ? comment.body : '').match(backportRegex);
        return match ? [match[1], new GHKey(ghKey.owner, ghKey.repo, parseInt(match[2]))] : null;
      };

      const comments = await getComments(ghKey);
      ((comments && comments.data) ? comments.data : [])
        .filter(isTropComment)
        .map(getBranchNameAndPullKey)
        .filter(pair => pair)
        .filter(([branch]) => mergedBranches.has(branch))
        .forEach(([branch, key]) => branches.set(branch, key));
    }
  }

  return branches;
}

// @返回`ref`所在分支的速记名称。
// 例如，‘10.0.0-beta.1’的ref将返回‘10-x-y’
async function getBranchNameOfRef (ref, dir) {
  return (await runGit(dir, ['branch', '--all', '--contains', ref, '--sort', 'version:refname']))
    .split(/\r?\n/) // 拆分成行。
    .shift() // 我们按重命名排序，并想要第一个结果。
    .match(/(?:\s?\*\s){0,1}(.*)/)[1] // 如果存在，请删除前导‘*’，以防我们当前在该分支中。
    .match(/(?:.*\/)?(.*)/)[1] // ‘Remote/Origins/10-x-y’-&gt;‘10-x-y’
    .trim();
}

/* *Main**。*/

const getNotes = async (fromRef, toRef, newVersion) => {
  const cacheDir = getCacheDir();
  if (!fs.existsSync(cacheDir)) {
    fs.mkdirSync(cacheDir);
  }

  const pool = new Pool();
  const toBranch = await getBranchNameOfRef(toRef, ELECTRON_DIR);

  console.log(`Generating release notes between '${fromRef}' and '${toRef}' for version '${newVersion}' in branch '${toBranch}'`);

  // 获得电子/电子承诺
  const electron = { owner: 'electron', repo: 'electron', dir: ELECTRON_DIR };
  await addRepoToPool(pool, electron, fromRef, toRef);

  // 删除所有旧提交。
  pool.commits = pool.commits.filter(commit => !pool.processedHashes.has(commit.hash));

  // 如果未处理的集合中出现COMMIT_AND_REVERT，请跳过它们。
  for (const commit of pool.commits) {
    const revertHash = commit.revertHash;
    if (!revertHash) {
      continue;
    }

    const revert = pool.commits.find(commit => commit.hash === revertHash);
    if (!revert) {
      continue;
    }

    commit.note = NO_NOTES;
    revert.note = NO_NOTES;
    pool.processedHashes.add(commit.hash);
    pool.processedHashes.add(revertHash);
  }

  // 确保提交有注释。
  for (const commit of pool.commits) {
    for (const prKey of commit.prKeys.values()) {
      if (commit.note) {
        break;
      }
      commit.note = await getNoteFromClerk(prKey);
    }
  }

  // 删除非面向用户的提交。
  pool.commits = pool.commits
    .filter(commit => commit && commit.note)
    .filter(commit => commit.note !== NO_NOTES)
    .filter(commit => commit.note.match(/^[Bb]ump v\d+\.\d+\.\d+/) === null);

  for (const commit of pool.commits) {
    commit.trops = await getMergedTrops(commit, pool);
  }

  pool.commits = removeSupercededStackUpdates(pool.commits);

  const notes = {
    breaking: [],
    docs: [],
    feat: [],
    fix: [],
    other: [],
    unknown: [],
    name: newVersion,
    toBranch
  };

  pool.commits.forEach(commit => {
    const str = commit.semanticType;
    if (commit.isBreakingChange) {
      notes.breaking.push(commit);
    } else if (!str) {
      notes.unknown.push(commit);
    } else if (docTypes.has(str)) {
      notes.docs.push(commit);
    } else if (featTypes.has(str)) {
      notes.feat.push(commit);
    } else if (fixTypes.has(str)) {
      notes.fix.push(commit);
    } else if (otherTypes.has(str)) {
      notes.other.push(commit);
    } else {
      notes.unknown.push(commit);
    }
  });

  return notes;
};

const removeSupercededStackUpdates = (commits) => {
  const updateRegex = /^Updated ([a-zA-Z.]+) to v?([\d.]+)/;
  const notupdates = [];

  const newest = {};
  for (const commit of commits) {
    const match = (commit.note || commit.subject).match(updateRegex);
    if (!match) {
      notupdates.push(commit);
      continue;
    }
    const [, dep, version] = match;
    if (!newest[dep] || newest[dep].version < version) {
      newest[dep] = { commit, version };
    }
  }

  return [...notupdates, ...Object.values(newest).map(o => o.commit)];
};

/* *渲染**。*/

// @返回拉流请求的GitHub地址。
const buildPullURL = ghKey => `https:// Github.com/${ghKey.owner}/${ghKey.repo}/pull/${ghKey.number}`；

const renderPull = ghKey => `[#${ghKey.number}](${buildPullURL(ghKey)})`;

// @返回提交的GitHub URL。
const buildCommitURL = commit => `https:// Github.com/${commit.owner}/${commit.repo}/commit/${commit.hash}`；

const renderCommit = commit => `[${commit.hash.slice(0, 8)}](${buildCommitURL(commit)})`;

// @返回PR的降价链接(如果可用)；否则，GIT提交。
function renderLink (commit) {
  const maybePull = commit.prKeys.values().next();
  return maybePull.value ? renderPull(maybePull.value) : renderCommit(commit);
}

// @返回更简短的分支机构名称，
// 例如‘7-2-x’-&gt;‘7.2’和‘8-x-y’-&gt;‘8’
const renderBranchName = name => name.replace(/-[a-zA-Z]/g, '').replace('-', '.');

const renderTrop = (branch, ghKey) => `[${renderBranchName(branch)}](${buildPullURL(ghKey)})`;

// @Return Markdown格式的链接指向其他分支机构的TROP，
// 例如“(也在7.2、8、9中)”
function renderTrops (commit, excludeBranch) {
  const body = [...commit.trops.entries()]
    .filter(([branch]) => branch !== excludeBranch)
    .sort(([branchA], [branchB]) => parseInt(branchA) - parseInt(branchB)) // 按半专业排序。
    .map(([branch, key]) => renderTrop(branch, key))
    .join(', ');
  return body ? `<span style="font-size:small;">(Also in ${body})</span>` : body;
}

// @返回稍加整理的人类可读的更改描述。
function renderDescription (commit) {
  let note = commit.note || commit.subject || '';
  note = note.trim();

  // 版本注释强调每个更改，因此如果注释作者。
  // 手动使用项目符号开始内容，这会让人感到困惑。
  // 标记下渲染器--去掉多余的项目符号。
  // (示例：https://github.com/electron/electron/pull/25216)。
  if (note.startsWith('*')) {
    note = note.slice(1).trim();
  }

  if (note.length !== 0) {
    note = note[0].toUpperCase() + note.substr(1);

    if (!note.endsWith('.')) {
      note = note + '.';
    }

    const commonVerbs = {
      Added: ['Add'],
      Backported: ['Backport'],
      Cleaned: ['Clean'],
      Disabled: ['Disable'],
      Ensured: ['Ensure'],
      Exported: ['Export'],
      Fixed: ['Fix', 'Fixes'],
      Handled: ['Handle'],
      Improved: ['Improve'],
      Made: ['Make'],
      Removed: ['Remove'],
      Repaired: ['Repair'],
      Reverted: ['Revert'],
      Stopped: ['Stop'],
      Updated: ['Update'],
      Upgraded: ['Upgrade']
    };
    for (const [key, values] of Object.entries(commonVerbs)) {
      for (const value of values) {
        const start = `${value} `;
        if (note.startsWith(start)) {
          note = `${key} ${note.slice(start.length)}`;
        }
      }
    }
  }

  return note;
}

// @Return降价格式的发行说明行项目，
// 例如‘*修复了foo。#12345(也在7.2、8、9中)‘。
const renderNote = (commit, excludeBranch) =>
  `* ${renderDescription(commit)} ${renderLink(commit)} ${renderTrops(commit, excludeBranch)}\n`;

const renderNotes = (notes) => {
  const rendered = [`# Release Notes for ${notes.name}\n\n`];

  const renderSection = (title, commits) => {
    if (commits.length > 0) {
      rendered.push(
        `## ${title}\n\n`,
        ...(commits.map(commit => renderNote(commit, notes.toBranch)).sort())
      );
    }
  };

  renderSection('Breaking Changes', notes.breaking);
  renderSection('Features', notes.feat);
  renderSection('Fixes', notes.fix);
  renderSection('Other Changes', notes.other);

  if (notes.docs.length) {
    const docs = notes.docs.map(commit => renderLink(commit)).sort();
    rendered.push('## Documentation\n\n', ` * Documentation changes: ${docs.join(', ')}\n`, '\n');
  }

  renderSection('Unknown', notes.unknown);

  return rendered.join('');
};

/* *模块***/

module.exports = {
  get: getNotes,
  render: renderNotes
};
