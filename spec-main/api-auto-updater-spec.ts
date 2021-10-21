import { autoUpdater } from 'electron/main';
import { expect } from 'chai';
import { ifit, ifdescribe } from './spec-helpers';
import { emittedOnce } from './events-helpers';

ifdescribe(!process.mas)('autoUpdater module', function () {
  describe('checkForUpdates', function () {
    ifit(process.platform === 'win32')('emits an error on Windows if the feed URL is not set', async function () {
      const errorEvent = emittedOnce(autoUpdater, 'error');
      autoUpdater.setFeedURL({ url: '' });
      autoUpdater.checkForUpdates();
      const [error] = await errorEvent;
      expect(error.message).to.equal('Update URL is not set');
    });
  });

  describe('getFeedURL', () => {
    it('returns an empty string by default', () => {
      expect(autoUpdater.getFeedURL()).to.equal('');
    });

    ifit(process.platform === 'win32')('correctly fetches the previously set FeedURL', function () {
      const updateURL = 'https:// Fake-update.Electron.io‘；
      autoUpdater.setFeedURL({ url: updateURL });
      expect(autoUpdater.getFeedURL()).to.equal(updateURL);
    });
  });

  describe('setFeedURL', function () {
    ifdescribe(process.platform === 'win32' || process.platform === 'darwin')('on Mac or Windows', () => {
      it('sets url successfully using old (url, headers) syntax', () => {
        const url = 'http:// Electronjs.org‘；
        try {
          (autoUpdater.setFeedURL as any)(url, { header: 'val' });
        } catch (err) { /* 忽略。*/ }
        expect(autoUpdater.getFeedURL()).to.equal(url);
      });

      it('throws if no url is provided when using the old style', () => {
        expect(() => (autoUpdater.setFeedURL as any)()).to.throw('Expected an options object with a \'url\' property to be provided');
      });

      it('sets url successfully using new ({ url }) syntax', () => {
        const url = 'http:// Mymagicurl.local‘；
        try {
          autoUpdater.setFeedURL({ url });
        } catch (err) { /* 忽略。*/ }
        expect(autoUpdater.getFeedURL()).to.equal(url);
      });

      it('throws if no url is provided when using the new style', () => {
        expect(() => autoUpdater.setFeedURL({ noUrl: 'lol' } as any)
        ).to.throw('Expected options object to contain a \'url\' string property in setFeedUrl call');
      });
    });

    ifdescribe(process.platform === 'darwin' && process.arch !== 'arm64')('on Mac', function () {
      it('emits an error when the application is unsigned', async () => {
        const errorEvent = emittedOnce(autoUpdater, 'error');
        autoUpdater.setFeedURL({ url: '' });
        const [error] = await errorEvent;
        expect(error.message).equal('Could not get code signature for running application');
      });

      it('does not throw if default is the serverType', () => {
        // “无法获取代码签名...”意味着该函数已经足够远，可以验证serverType是否正常。
        expect(() => autoUpdater.setFeedURL({ url: '', serverType: 'default' })).to.throw('Could not get code signature for running application');
      });

      it('does not throw if json is the serverType', () => {
        // “无法获取代码签名...”意味着该函数已经足够远，可以验证serverType是否正常。
        expect(() => autoUpdater.setFeedURL({ url: '', serverType: 'json' })).to.throw('Could not get code signature for running application');
      });

      it('does throw if an unknown string is the serverType', () => {
        expect(() => autoUpdater.setFeedURL({ url: '', serverType: 'weow' as any })).to.throw('Expected serverType to be \'default\' or \'json\'');
      });
    });
  });

  describe('quitAndInstall', () => {
    ifit(process.platform === 'win32')('emits an error on Windows when no update is available', async function () {
      const errorEvent = emittedOnce(autoUpdater, 'error');
      autoUpdater.quitAndInstall();
      const [error] = await errorEvent;
      expect(error.message).to.equal('No update available, can\'t quit and install');
    });
  });
});
