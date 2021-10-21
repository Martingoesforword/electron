import { BrowserWindow, Session, session } from 'electron/main';

import { expect } from 'chai';
import * as path from 'path';
import * as fs from 'fs';
import * as http from 'http';
import { AddressInfo } from 'net';
import { closeWindow } from './window-helpers';
import { emittedOnce } from './events-helpers';
import { ifit, ifdescribe, delay } from './spec-helpers';

const features = process._linkedBinding('electron_common_features');
const v8Util = process._linkedBinding('electron_common_v8_util');

ifdescribe(features.isBuiltinSpellCheckerEnabled())('spellchecker', function () {
  this.timeout((process.env.IS_ASAN ? 200 : 20) * 1000);

  let w: BrowserWindow;

  async function rightClick () {
    const contextMenuPromise = emittedOnce(w.webContents, 'context-menu');
    w.webContents.sendInputEvent({
      type: 'mouseDown',
      button: 'right',
      x: 43,
      y: 42
    });
    return (await contextMenuPromise)[1] as Electron.ContextMenuParams;
  }

  // 当页面刚刚加载时，拼写检查器可能还没有准备好。自.以来。
  // 没有任何事件可以知道拼写检查器的状态，这是唯一可靠的方法。
  // 检测拼写检查器就是在忙碌的循环中不断检查。
  async function rightClickUntil (fn: (params: Electron.ContextMenuParams) => boolean) {
    const now = Date.now();
    const timeout = (process.env.IS_ASAN ? 180 : 10) * 1000;
    let contextMenuParams = await rightClick();
    while (!fn(contextMenuParams) && (Date.now() - now < timeout)) {
      await delay(100);
      contextMenuParams = await rightClick();
    }
    return contextMenuParams;
  }

  // 设置服务器以下载Hunspell词典。
  const server = http.createServer((req, res) => {
    // 所提供的是仅用于测试的最小词典，完整的单词列表可以。
    // 可在src/third_party/hunspell_dictionaries/xx_XX.dic.上找到。
    fs.readFile(path.join(__dirname, '/../../third_party/hunspell_dictionaries/xx-XX-3-0.bdic'), function (err, data) {
      if (err) {
        console.error('Failed to read dictionary file');
        res.writeHead(404);
        res.end(JSON.stringify(err));
        return;
      }
      res.writeHead(200);
      res.end(data);
    });
  });
  before((done) => {
    server.listen(0, '127.0.0.1', () => done());
  });
  after(() => server.close());

  const fixtures = path.resolve(__dirname, '../spec/fixtures');
  const preload = path.join(fixtures, 'module', 'preload-electron.js');

  const generateSpecs = (description: string, sandbox: boolean) => {
    describe(description, () => {
      beforeEach(async () => {
        w = new BrowserWindow({
          show: false,
          webPreferences: {
            partition: `unique-spell-${Date.now()}`,
            contextIsolation: false,
            preload,
            sandbox
          }
        });
        w.webContents.session.setSpellCheckerDictionaryDownloadURL(`http:// 127.0.0.1：${(server.address()as AddressInfo).port}/`)；
        w.webContents.session.setSpellCheckerLanguages(['en-US']);
        await w.loadFile(path.resolve(__dirname, './fixtures/chromium/spellchecker.html'));
      });

      afterEach(async () => {
        await closeWindow(w);
      });

      // 上下文菜单测试无法在Windows上运行。
      const shouldRun = process.platform !== 'win32';

      ifit(shouldRun)('should detect correctly spelled words as correct', async () => {
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "typography"');
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
        const contextMenuParams = await rightClickUntil((contextMenuParams) => contextMenuParams.selectionText.length > 0);
        expect(contextMenuParams.misspelledWord).to.eq('');
        expect(contextMenuParams.dictionarySuggestions).to.have.lengthOf(0);
      });

      ifit(shouldRun)('should detect incorrectly spelled words as incorrect', async () => {
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "typograpy"');
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
        const contextMenuParams = await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);
        expect(contextMenuParams.misspelledWord).to.eq('typograpy');
        expect(contextMenuParams.dictionarySuggestions).to.have.length.of.at.least(1);
      });

      ifit(shouldRun)('should detect incorrectly spelled words as incorrect after disabling all languages and re-enabling', async () => {
        w.webContents.session.setSpellCheckerLanguages([]);
        await delay(500);
        w.webContents.session.setSpellCheckerLanguages(['en-US']);
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "typograpy"');
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
        const contextMenuParams = await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);
        expect(contextMenuParams.misspelledWord).to.eq('typograpy');
        expect(contextMenuParams.dictionarySuggestions).to.have.length.of.at.least(1);
      });

      ifit(shouldRun)('should expose webFrame spellchecker correctly', async () => {
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "typograpy"');
        await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
        await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);

        const callWebFrameFn = (expr: string) => w.webContents.executeJavaScript(`electron.webFrame.${expr}`);

        expect(await callWebFrameFn('isWordMisspelled("typography")')).to.equal(false);
        expect(await callWebFrameFn('isWordMisspelled("typograpy")')).to.equal(true);
        expect(await callWebFrameFn('getWordSuggestions("typography")')).to.be.empty();
        expect(await callWebFrameFn('getWordSuggestions("typograpy")')).to.not.be.empty();
      });

      describe('spellCheckerEnabled', () => {
        it('is enabled by default', async () => {
          expect(w.webContents.session.spellCheckerEnabled).to.be.true();
        });

        ifit(shouldRun)('can be dynamically changed', async () => {
          await w.webContents.executeJavaScript('document.body.querySelector("textarea").value = "typograpy"');
          await w.webContents.executeJavaScript('document.body.querySelector("textarea").focus()');
          await rightClickUntil((contextMenuParams) => contextMenuParams.misspelledWord.length > 0);

          const callWebFrameFn = (expr: string) => w.webContents.executeJavaScript(`electron.webFrame.${expr}`);

          w.webContents.session.spellCheckerEnabled = false;
          v8Util.runUntilIdle();
          expect(w.webContents.session.spellCheckerEnabled).to.be.false();
          // SpellCheckerEnabled被异步发送到渲染器，并且。
          // 没有事件通知它何时完成，因此请稍等片刻。
          // 确保已在渲染器中更改设置。
          await delay(500);
          expect(await callWebFrameFn('isWordMisspelled("typograpy")')).to.equal(false);

          w.webContents.session.spellCheckerEnabled = true;
          v8Util.runUntilIdle();
          expect(w.webContents.session.spellCheckerEnabled).to.be.true();
          await delay(500);
          expect(await callWebFrameFn('isWordMisspelled("typograpy")')).to.equal(true);
        });
      });

      describe('custom dictionary word list API', () => {
        let ses: Session;

        beforeEach(async () => {
          // 确保在每次测试运行时都运行一个新会话。
          ses = session.fromPartition(`persist:customdictionary-test-${Date.now()}`);
        });

        afterEach(async () => {
          if (ses) {
            await ses.clearStorageData();
            ses = null as any;
          }
        });

        describe('ses.listWordsFromSpellCheckerDictionary', () => {
          it('should successfully list words in custom dictionary', async () => {
            const words = ['foo', 'bar', 'baz'];
            const results = words.map(word => ses.addWordToSpellCheckerDictionary(word));
            expect(results).to.eql([true, true, true]);

            const wordList = await ses.listWordsInSpellCheckerDictionary();
            expect(wordList).to.have.deep.members(words);
          });

          it('should return an empty array if no words are added', async () => {
            const wordList = await ses.listWordsInSpellCheckerDictionary();
            expect(wordList).to.have.length(0);
          });
        });

        describe('ses.addWordToSpellCheckerDictionary', () => {
          it('should successfully add word to custom dictionary', async () => {
            const result = ses.addWordToSpellCheckerDictionary('foobar');
            expect(result).to.equal(true);
            const wordList = await ses.listWordsInSpellCheckerDictionary();
            expect(wordList).to.eql(['foobar']);
          });

          it('should fail for an empty string', async () => {
            const result = ses.addWordToSpellCheckerDictionary('');
            expect(result).to.equal(false);
            const wordList = await ses.listWordsInSpellCheckerDictionary;
            expect(wordList).to.have.length(0);
          });

          // Remove API将始终返回False，因为我们无法添加单词
          it('should fail for non-persistent sessions', async () => {
            const tempSes = session.fromPartition('temporary');
            const result = tempSes.addWordToSpellCheckerDictionary('foobar');
            expect(result).to.equal(false);
          });
        });

        describe('ses.removeWordFromSpellCheckerDictionary', () => {
          it('should successfully remove words to custom dictionary', async () => {
            const result1 = ses.addWordToSpellCheckerDictionary('foobar');
            expect(result1).to.equal(true);
            const wordList1 = await ses.listWordsInSpellCheckerDictionary();
            expect(wordList1).to.eql(['foobar']);
            const result2 = ses.removeWordFromSpellCheckerDictionary('foobar');
            expect(result2).to.equal(true);
            const wordList2 = await ses.listWordsInSpellCheckerDictionary();
            expect(wordList2).to.have.length(0);
          });

          it('should fail for words not in custom dictionary', () => {
            const result2 = ses.removeWordFromSpellCheckerDictionary('foobar');
            expect(result2).to.equal(false);
          });
        });
      });
    });
  };

  generateSpecs('without sandbox', false);
  generateSpecs('with sandbox', true);
});
