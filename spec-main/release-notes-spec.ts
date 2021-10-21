import { GitProcess, IGitExecutionOptions, IGitResult } from 'dugite';
import { expect } from 'chai';
import * as notes from '../script/release/notes/notes.js';
import * as path from 'path';
import * as sinon from 'sinon';

/* 伪造只返回我们要测试的特定提交的Dugite GitProcess。*/

class Commit {
  sha1: string;
  subject: string;
  constructor (sha1: string, subject: string) {
    this.sha1 = sha1;
    this.subject = subject;
  }
}

class GitFake {
  branches: {
    [key: string]: Commit[],
  };

  constructor () {
    this.branches = {};
  }

  setBranch (name: string, commits: Array<Commit>): void {
    this.branches[name] = commits;
  }

  // 查找分支a和b之间的最新共享提交。
  mergeBase (a: string, b:string): string {
    for (const commit of [...this.branches[a].reverse()]) {
      if (this.branches[b].map((commit: Commit) => commit.sha1).includes(commit.sha1)) {
        return commit.sha1;
      }
    }
    console.error('test error: branches not related');
    return '';
  }

  // Eslint-disable-next-line@tyescript-eslint/no-unuse-vars。
  exec (args: string[], path: string, options?: IGitExecutionOptions | undefined): Promise<IGitResult> {
    let stdout = '';
    const stderr = '';
    const exitCode = 0;

    if (args.length === 3 && args[0] === 'merge-base') {
      // 预期格式：`git合并基础分支名称1分支名称2`。
      const a: string = args[1]!;
      const b: string = args[2]!;
      stdout = this.mergeBase(a, b);
    } else if (args.length === 3 && args[0] === 'log' && args[1] === '--format=%H') {
      // Exepcted Form：`git日志--格式=%H分支名称。
      const branch: string = args[2]!;
      stdout = this.branches[branch].map((commit: Commit) => commit.sha1).join('\n');
    } else if (args.length > 1 && args[0] === 'log' && args.includes('--format=%H,%s')) {
      // 预期格式：`git日志--格式=%H，%s SHA1..分支名称。
      const [start, branch] = args[args.length - 1].split('..');
      const lines : string[] = [];
      let started = false;
      for (const commit of this.branches[branch]) {
        started = started || commit.sha1 === start;
        if (started) {
          lines.push(`${commit.sha1},${commit.subject}` /* %H，%s。*/);
        }
      }
      stdout = lines.join('\n');
    } else if (args.length === 6 &&
               args[0] === 'branch' &&
               args[1] === '--all' &&
               args[2] === '--contains' &&
               args[3].endsWith('-x-y')) {
      // “这个标签在哪个分支机构？”
      // Git分支--全部--包含${ref}--排序版本：refname。
      stdout = args[3];
    } else {
      console.error('unhandled GitProcess.exec():', args);
    }

    return Promise.resolve({ exitCode, stdout, stderr });
  }
}

describe('release notes', () => {
  const sandbox = sinon.createSandbox();
  const gitFake = new GitFake();

  const oldBranch = '8-x-y';
  const newBranch = '9-x-y';

  // 由oldBranch和newBranch共享的提交。
  const sharedHistory = [
    new Commit('2abea22b4bffa1626a521711bacec7cd51425818', "fix: explicitly cancel redirects when mode is 'error' (#20686)"),
    new Commit('467409458e716c68b35fa935d556050ca6bed1c4', 'build: add support for automated minor releases (#20620)') // 合并基数。
  ];

  // 这些提交发生在创建newBranch之后。
  const newBreaking = new Commit('2fad53e66b1a2cb6f7dad88fe9bb62d7a461fe98', 'refactor: use v8 serialization for ipc (#20214)');
  const newFeat = new Commit('89eb309d0b22bd4aec058ffaf983e81e56a5c378', 'feat: allow GUID parameter to avoid systray demotion on Windows  (#21891)');
  const newFix = new Commit('0600420bac25439fc2067d51c6aaa4ee11770577', "fix: don't allow window to go behind menu bar on mac (#22828)");
  const oldFix = new Commit('f77bd19a70ac2d708d17ddbe4dc12745ca3a8577', 'fix: prevent menu gc during popup (#20785)');

  // 通过单独的PR在两个分支中修复的错误。
  const newTropFix = new Commit('a6ff42c190cb5caf8f3e217748e49183a951491b', 'fix: workaround for hang when preventDefault-ing nativeWindowOpen (#22750)');
  const oldTropFix = new Commit('8751f485c5a6c8c78990bfd55a4350700f81f8cd', 'fix: workaround for hang when preventDefault-ing nativeWindowOpen (#22749)');

  // 具有异常注释格式的公关。
  const sublist = new Commit('61dc1c88fd34a3e8fff80c80ed79d0455970e610', 'fix: client area inset calculation when maximized for framless windows (#25052) (#25216)');

  before(() => {
    // Relase-Notes的octokit回复缓存的位置。
    const fixtureDir = path.resolve(__dirname, 'fixtures', 'release-notes');
    process.env.NOTES_CACHE_PATH = path.resolve(fixtureDir, 'cache');
  });

  beforeEach(() => {
    const wrapper = (args: string[], path: string, options?: IGitExecutionOptions | undefined) => gitFake.exec(args, path, options);
    sandbox.replace(GitProcess, 'exec', wrapper);

    gitFake.setBranch(oldBranch, [...sharedHistory, oldFix]);
  });

  afterEach(() => {
    sandbox.restore();
  });

  describe('trop annotations', () => {
    it('shows sibling branches', async function () {
      const version = 'v9.0.0';
      gitFake.setBranch(oldBranch, [...sharedHistory, oldTropFix]);
      gitFake.setBranch(newBranch, [...sharedHistory, newTropFix]);
      const results: any = await notes.get(oldBranch, newBranch, version);
      expect(results.fix).to.have.lengthOf(1);
      console.log(results.fix);
      expect(results.fix[0].trops).to.have.keys('8-x-y', '9-x-y');
    });
  });

  // 用例：恶意贡献者可以编辑他们的“Notes：”的文本。
  // 在公关合并和维护者离开之后的公关机构中。
  // 因此，请始终使用发布员公关评论。
  it('uses the release-clerk text', async function () {
    // RealText来源：${fixtureDir}/electron-electron-issue-21891-comments。
    const realText = 'Added GUID parameter to Tray API to avoid system tray icon demotion on Windows';
    const testCommit = new Commit('89eb309d0b22bd4aec058ffaf983e81e56a5c378', 'feat: lole u got troled hard (#21891)');
    const version = 'v9.0.0';

    gitFake.setBranch(newBranch, [...sharedHistory, testCommit]);
    const results: any = await notes.get(oldBranch, newBranch, version);
    expect(results.feat).to.have.lengthOf(1);
    expect(results.feat[0].hash).to.equal(testCommit.sha1);
    expect(results.feat[0].note).to.equal(realText);
  });

  describe('rendering', () => {
    it('removes redundant bullet points', async function () {
      const testCommit = sublist;
      const version = 'v10.1.1';

      gitFake.setBranch(newBranch, [...sharedHistory, testCommit]);
      const results: any = await notes.get(oldBranch, newBranch, version);
      const rendered: any = await notes.render(results);

      expect(rendered).to.not.include('* *');
    });

    it('indents sublists', async function () {
      const testCommit = sublist;
      const version = 'v10.1.1';

      gitFake.setBranch(newBranch, [...sharedHistory, testCommit]);
      const results: any = await notes.get(oldBranch, newBranch, version);
      const rendered: any = await notes.render(results);

      expect(rendered).to.include([
        '* Fixed the following issues for frameless when maximized on Windows:',
        '  * fix unreachable task bar when auto hidden with position top',
        '  * fix 1px extending to secondary monitor',
        '  * fix 1px overflowing into taskbar at certain resolutions',
        '  * fix white line on top of window under 4k resolutions. [#25216]'].join('\n'));
    });
  });
  // 测试当您输入不同的语义提交类型时，
  // 解析器在结果的正确类别中返回它们
  describe('semantic commit', () => {
    const version = 'v9.0.0';

    it("honors 'feat' type", async function () {
      const testCommit = newFeat;
      gitFake.setBranch(newBranch, [...sharedHistory, testCommit]);
      const results: any = await notes.get(oldBranch, newBranch, version);
      expect(results.feat).to.have.lengthOf(1);
      expect(results.feat[0].hash).to.equal(testCommit.sha1);
    });

    it("honors 'fix' type", async function () {
      const testCommit = newFix;
      gitFake.setBranch(newBranch, [...sharedHistory, testCommit]);
      const results: any = await notes.get(oldBranch, newBranch, version);
      expect(results.fix).to.have.lengthOf(1);
      expect(results.fix[0].hash).to.equal(testCommit.sha1);
    });

    it("honors 'BREAKING CHANGE' message", async function () {
      const testCommit = newBreaking;
      gitFake.setBranch(newBranch, [...sharedHistory, testCommit]);
      const results: any = await notes.get(oldBranch, newBranch, version);
      expect(results.breaking).to.have.lengthOf(1);
      expect(results.breaking[0].hash).to.equal(testCommit.sha1);
    });
  });
});
