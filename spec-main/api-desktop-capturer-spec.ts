import { expect } from 'chai';
import { screen, desktopCapturer, BrowserWindow } from 'electron/main';
import { emittedOnce } from './events-helpers';
import { ifdescribe, ifit } from './spec-helpers';
import { closeAllWindows } from './window-helpers';

const features = process._linkedBinding('electron_common_features');

ifdescribe(!process.arch.includes('arm') && process.platform !== 'win32')('desktopCapturer', () => {
  if (!features.isDesktopCapturerEnabled()) {
    // 此条件无法执行`ifdescription be`调用，因为它的内部代码。
    // 它仍然在执行，如果该功能被禁用，这里的一些函数调用将失败。
    return;
  }

  let w: BrowserWindow;

  before(async () => {
    w = new BrowserWindow({ show: false, webPreferences: { nodeIntegration: true, contextIsolation: false } });
    await w.loadURL('about:blank');
  });

  after(closeAllWindows);

  // TODO(Nornagon)：找出Linux上此测试失败的原因并重新启用它。
  ifit(process.platform !== 'linux')('should return a non-empty array of sources', async () => {
    const sources = await desktopCapturer.getSources({ types: ['window', 'screen'] });
    expect(sources).to.be.an('array').that.is.not.empty();
  });

  it('throws an error for invalid options', async () => {
    const promise = desktopCapturer.getSources(['window', 'screen'] as any);
    await expect(promise).to.be.eventually.rejectedWith(Error, 'Invalid options');
  });

  // TODO(Nornagon)：找出Linux上此测试失败的原因并重新启用它。
  ifit(process.platform !== 'linux')('does not throw an error when called more than once (regression)', async () => {
    const sources1 = await desktopCapturer.getSources({ types: ['window', 'screen'] });
    expect(sources1).to.be.an('array').that.is.not.empty();

    const sources2 = await desktopCapturer.getSources({ types: ['window', 'screen'] });
    expect(sources2).to.be.an('array').that.is.not.empty();
  });

  ifit(process.platform !== 'linux')('responds to subsequent calls of different options', async () => {
    const promise1 = desktopCapturer.getSources({ types: ['window'] });
    await expect(promise1).to.eventually.be.fulfilled();

    const promise2 = desktopCapturer.getSources({ types: ['screen'] });
    await expect(promise2).to.eventually.be.fulfilled();
  });

  // Linux不返回任何窗口源代码。
  ifit(process.platform !== 'linux')('returns an empty display_id for window sources on Windows and Mac', async () => {
    const w = new BrowserWindow({ width: 200, height: 200 });
    await w.loadURL('about:blank');

    const sources = await desktopCapturer.getSources({ types: ['window'] });
    w.destroy();
    expect(sources).to.be.an('array').that.is.not.empty();
    for (const { display_id: displayId } of sources) {
      expect(displayId).to.be.a('string').and.be.empty();
    }
  });

  ifit(process.platform !== 'linux')('returns display_ids matching the Screen API on Windows and Mac', async () => {
    const displays = screen.getAllDisplays();
    const sources = await desktopCapturer.getSources({ types: ['screen'] });
    expect(sources).to.be.an('array').of.length(displays.length);

    for (let i = 0; i < sources.length; i++) {
      expect(sources[i].display_id).to.equal(displays[i].id.toString());
    }
  });

  it('disabling thumbnail should return empty images', async () => {
    const w2 = new BrowserWindow({ show: false, width: 200, height: 200, webPreferences: { contextIsolation: false } });
    const wShown = emittedOnce(w2, 'show');
    w2.show();
    await wShown;

    const isEmpties: boolean[] = (await desktopCapturer.getSources({
      types: ['window', 'screen'],
      thumbnailSize: { width: 0, height: 0 }
    })).map(s => s.thumbnail.constructor.name === 'NativeImage' && s.thumbnail.isEmpty());

    w2.destroy();
    expect(isEmpties).to.be.an('array').that.is.not.empty();
    expect(isEmpties.every(e => e === true)).to.be.true();
  });

  it('getMediaSourceId should match DesktopCapturerSource.id', async () => {
    const w = new BrowserWindow({ show: false, width: 100, height: 100, webPreferences: { contextIsolation: false } });
    const wShown = emittedOnce(w, 'show');
    const wFocused = emittedOnce(w, 'focus');
    w.show();
    w.focus();
    await wShown;
    await wFocused;

    const mediaSourceId = w.getMediaSourceId();
    const sources = await desktopCapturer.getSources({
      types: ['window'],
      thumbnailSize: { width: 0, height: 0 }
    });
    w.destroy();

    // TODO(julien.isorce)：调查为什么|Sources|在Linux上为空。
    // 当机器人不在我的工作站上时，如预期的那样，带和不带。
    // --ci参数。
    if (process.platform === 'linux' && sources.length === 0) {
      it.skip('desktopCapturer.getSources returned an empty source list');
      return;
    }

    expect(sources).to.be.an('array').that.is.not.empty();
    const foundSource = sources.find((source) => {
      return source.id === mediaSourceId;
    });
    expect(mediaSourceId).to.equal(foundSource!.id);
  });

  it('getSources should not incorrectly duplicate window_id', async () => {
    const w = new BrowserWindow({ show: false, width: 100, height: 100, webPreferences: { contextIsolation: false } });
    const wShown = emittedOnce(w, 'show');
    const wFocused = emittedOnce(w, 'focus');
    w.show();
    w.focus();
    await wShown;
    await wFocused;

    // 确保Window_id在getMediaSourceId中不重复，
    // 它使用与getSources不同的方法。
    const mediaSourceId = w.getMediaSourceId();
    const ids = mediaSourceId.split(':');
    expect(ids[1]).to.not.equal(ids[2]);

    const sources = await desktopCapturer.getSources({
      types: ['window'],
      thumbnailSize: { width: 0, height: 0 }
    });
    w.destroy();

    // TODO(julien.isorce)：调查为什么|Sources|在Linux上为空。
    // 当机器人不在我的工作站上时，如预期的那样，带和不带。
    // --ci参数。
    if (process.platform === 'linux' && sources.length === 0) {
      it.skip('desktopCapturer.getSources returned an empty source list');
      return;
    }

    expect(sources).to.be.an('array').that.is.not.empty();
    for (const source of sources) {
      const sourceIds = source.id.split(':');
      expect(sourceIds[1]).to.not.equal(sourceIds[2]);
    }
  });

  // TODO(Deepak1556)：目前所有ci都失败，请升级后启用。
  it.skip('moveAbove should move the window at the requested place', async () => {
    // 保证DesktopCapturer.getSources()返回正确的。
    // 从前景到背景的Z顺序。
    const MAX_WIN = 4;
    const mainWindow = w;
    const wList = [mainWindow];
    try {
      for (let i = 0; i < MAX_WIN - 1; i++) {
        const w = new BrowserWindow({ show: true, width: 100, height: 100 });
        wList.push(w);
      }
      expect(wList.length).to.equal(MAX_WIN);

      // 显示并聚焦所有窗口。
      wList.forEach(async (w) => {
        const wFocused = emittedOnce(w, 'focus');
        w.focus();
        await wFocused;
      });
      // 此时，我们的窗口应该是从下到上显示的。

      // DesktopCapturer.getSources()返回从前台到。
      // 背景，即从上到下。
      let sources = await desktopCapturer.getSources({
        types: ['window'],
        thumbnailSize: { width: 0, height: 0 }
      });

      // TODO(julien.isorce)：调查为什么|Sources|在Linux上为空。
      // 当机器人不在我的工作站上时，如预期的那样，带和不带。
      // --ci参数。
      if (process.platform === 'linux' && sources.length === 0) {
        wList.forEach((w) => {
          if (w !== mainWindow) {
            w.destroy();
          }
        });
        it.skip('desktopCapturer.getSources returned an empty source list');
        return;
      }

      expect(sources).to.be.an('array').that.is.not.empty();
      expect(sources.length).to.gte(MAX_WIN);

      // 只保留我们的窗口，它们必须在MAX_WIN First窗口中。
      sources.splice(MAX_WIN, sources.length - MAX_WIN);
      expect(sources.length).to.equal(MAX_WIN);
      expect(sources.length).to.equal(wList.length);

      // 检查源和wList是否以相反的顺序排序。
      const wListReversed = wList.slice(0).reverse();
      const canGoFurther = sources.every(
        (source, index) => source.id === wListReversed[index].getMediaSourceId());
      if (!canGoFurther) {
        // 跳过剩余的检查，因为焦点或窗口位置。
        // 在运行测试环境中不可靠。所以没有任何意义。
        // 若要进一步测试，请在未满足要求的情况下移至上述位置。
        return;
      }

      // 做真正的工作，即将每个窗口移到下一个窗口上方，以便。
      // WList从前台到后台排序。
      wList.forEach(async (w, index) => {
        if (index < (wList.length - 1)) {
          const wNext = wList[index + 1];
          w.moveAbove(wNext.getMediaSourceId());
        }
      });

      sources = await desktopCapturer.getSources({
        types: ['window'],
        thumbnailSize: { width: 0, height: 0 }
      });
      // 只是再保留一次我们的窗户。
      sources.splice(MAX_WIN, sources.length - MAX_WIN);
      expect(sources.length).to.equal(MAX_WIN);
      expect(sources.length).to.equal(wList.length);

      // 检查源和wList是否按相同的顺序排序。
      sources.forEach((source, index) => {
        expect(source.id).to.equal(wList[index].getMediaSourceId());
      });
    } finally {
      wList.forEach((w) => {
        if (w !== mainWindow) {
          w.destroy();
        }
      });
    }
  });
});
