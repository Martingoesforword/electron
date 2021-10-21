import { expect } from 'chai';
import * as http from 'http';
import * as path from 'path';
import * as url from 'url';
import { BrowserWindow, WebFrameMain, webFrameMain, ipcMain } from 'electron/main';
import { closeAllWindows } from './window-helpers';
import { emittedOnce, emittedNTimes } from './events-helpers';
import { AddressInfo } from 'net';
import { waitUntil } from './spec-helpers';

describe('webFrameMain module', () => {
  const fixtures = path.resolve(__dirname, '..', 'spec-main', 'fixtures');
  const subframesPath = path.join(fixtures, 'sub-frames');

  const fileUrl = (filename: string) => url.pathToFileURL(path.join(subframesPath, filename)).href;

  type Server = { server: http.Server, url: string }

  /* *创建一个HTTP服务器，其处理程序嵌入给定的iframe src。*/
  const createServer = () => new Promise<Server>(resolve => {
    const server = http.createServer((req, res) => {
      const params = new URLSearchParams(url.parse(req.url || '').search || '');
      if (params.has('frameSrc')) {
        res.end(`<iframe src="${params.get('frameSrc')}"></iframe>`);
      } else {
        res.end('');
      }
    });
    server.listen(0, '127.0.0.1', () => {
      const url = `http:// 127.0.0.1：${(server.address()as AddressInfo).port}/`；
      resolve({ server, url });
    });
  });

  afterEach(closeAllWindows);

  describe('WebFrame traversal APIs', () => {
    let w: BrowserWindow;
    let webFrame: WebFrameMain;

    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      webFrame = w.webContents.mainFrame;
    });

    it('can access top frame', () => {
      expect(webFrame.top).to.equal(webFrame);
    });

    it('has no parent on top frame', () => {
      expect(webFrame.parent).to.be.null();
    });

    it('can access immediate frame descendents', () => {
      const { frames } = webFrame;
      expect(frames).to.have.lengthOf(1);
      const subframe = frames[0];
      expect(subframe).not.to.equal(webFrame);
      expect(subframe.parent).to.equal(webFrame);
    });

    it('can access deeply nested frames', () => {
      const subframe = webFrame.frames[0];
      expect(subframe).not.to.equal(webFrame);
      expect(subframe.parent).to.equal(webFrame);
      const nestedSubframe = subframe.frames[0];
      expect(nestedSubframe).not.to.equal(webFrame);
      expect(nestedSubframe).not.to.equal(subframe);
      expect(nestedSubframe.parent).to.equal(subframe);
    });

    it('can traverse all frames in root', () => {
      const urls = webFrame.framesInSubtree.map(frame => frame.url);
      expect(urls).to.deep.equal([
        fileUrl('frame-with-frame-container.html'),
        fileUrl('frame-with-frame.html'),
        fileUrl('frame.html')
      ]);
    });

    it('can traverse all frames in subtree', () => {
      const urls = webFrame.frames[0].framesInSubtree.map(frame => frame.url);
      expect(urls).to.deep.equal([
        fileUrl('frame-with-frame.html'),
        fileUrl('frame.html')
      ]);
    });

    describe('cross-origin', () => {
      let serverA = null as unknown as Server;
      let serverB = null as unknown as Server;

      before(async () => {
        serverA = await createServer();
        serverB = await createServer();
      });

      after(() => {
        serverA.server.close();
        serverB.server.close();
      });

      it('can access cross-origin frames', async () => {
        await w.loadURL(`${serverA.url}?frameSrc=${serverB.url}`);
        webFrame = w.webContents.mainFrame;
        expect(webFrame.url.startsWith(serverA.url)).to.be.true();
        expect(webFrame.frames[0].url).to.equal(serverB.url);
      });
    });
  });

  describe('WebFrame.url', () => {
    it('should report correct address for each subframe', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const webFrame = w.webContents.mainFrame;

      expect(webFrame.url).to.equal(fileUrl('frame-with-frame-container.html'));
      expect(webFrame.frames[0].url).to.equal(fileUrl('frame-with-frame.html'));
      expect(webFrame.frames[0].frames[0].url).to.equal(fileUrl('frame.html'));
    });
  });

  describe('WebFrame IDs', () => {
    it('has properties for various identifiers', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = w.webContents.mainFrame;
      expect(webFrame).to.have.ownProperty('url').that.is.a('string');
      expect(webFrame).to.have.ownProperty('frameTreeNodeId').that.is.a('number');
      expect(webFrame).to.have.ownProperty('name').that.is.a('string');
      expect(webFrame).to.have.ownProperty('osProcessId').that.is.a('number');
      expect(webFrame).to.have.ownProperty('processId').that.is.a('number');
      expect(webFrame).to.have.ownProperty('routingId').that.is.a('number');
    });
  });

  describe('WebFrame.visibilityState', () => {
    it('should match window state', async () => {
      const w = new BrowserWindow({ show: true });
      await w.loadURL('about:blank');
      const webFrame = w.webContents.mainFrame;

      expect(webFrame.visibilityState).to.equal('visible');
      w.hide();
      await expect(
        waitUntil(() => webFrame.visibilityState === 'hidden')
      ).to.eventually.be.fulfilled();
    });
  });

  describe('WebFrame.executeJavaScript', () => {
    it('can inject code into any subframe', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const webFrame = w.webContents.mainFrame;

      const getUrl = (frame: WebFrameMain) => frame.executeJavaScript('location.href');
      expect(await getUrl(webFrame)).to.equal(fileUrl('frame-with-frame-container.html'));
      expect(await getUrl(webFrame.frames[0])).to.equal(fileUrl('frame-with-frame.html'));
      expect(await getUrl(webFrame.frames[0].frames[0])).to.equal(fileUrl('frame.html'));
    });
  });

  describe('WebFrame.reload', () => {
    it('reloads a frame', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
      await w.loadFile(path.join(subframesPath, 'frame.html'));
      const webFrame = w.webContents.mainFrame;

      await webFrame.executeJavaScript('window.TEMP = 1', false);
      expect(webFrame.reload()).to.be.true();
      await emittedOnce(w.webContents, 'dom-ready');
      expect(await webFrame.executeJavaScript('window.TEMP', false)).to.be.null();
    });
  });

  describe('WebFrame.send', () => {
    it('works', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          preload: path.join(subframesPath, 'preload.js'),
          nodeIntegrationInSubFrames: true
        }
      });
      await w.loadURL('about:blank');
      const webFrame = w.webContents.mainFrame;
      const pongPromise = emittedOnce(ipcMain, 'preload-pong');
      webFrame.send('preload-ping');
      const [, routingId] = await pongPromise;
      expect(routingId).to.equal(webFrame.routingId);
    });
  });

  describe('RenderFrame lifespan', () => {
    let w: BrowserWindow;

    beforeEach(async () => {
      w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });
    });

    it('throws upon accessing properties when disposed', async () => {
      await w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));
      const { mainFrame } = w.webContents;
      w.destroy();
      // 等待WebContents以及RenderFrameHost被销毁。
      await new Promise(resolve => setTimeout(resolve, 0));
      expect(() => mainFrame.url).to.throw();
    });

    it('persists through cross-origin navigation', async () => {
      const server = await createServer();
      // “localhost”被视为单独的来源。
      const crossOriginUrl = server.url.replace('127.0.0.1', 'localhost');
      await w.loadURL(server.url);
      const { mainFrame } = w.webContents;
      expect(mainFrame.url).to.equal(server.url);
      await w.loadURL(crossOriginUrl);
      expect(w.webContents.mainFrame).to.equal(mainFrame);
      expect(mainFrame.url).to.equal(crossOriginUrl);
    });
  });

  describe('webFrameMain.fromId', () => {
    it('returns undefined for unknown IDs', () => {
      expect(webFrameMain.fromId(0, 0)).to.be.undefined();
    });

    it('can find each frame from navigation events', async () => {
      const w = new BrowserWindow({ show: false, webPreferences: { contextIsolation: true } });

      // Frame-with-Frame-tainer.html、Frame-with-Frame.html、Frame.html。
      const didFrameFinishLoad = emittedNTimes(w.webContents, 'did-frame-finish-load', 3);
      w.loadFile(path.join(subframesPath, 'frame-with-frame-container.html'));

      for (const [, isMainFrame, frameProcessId, frameRoutingId] of await didFrameFinishLoad) {
        const frame = webFrameMain.fromId(frameProcessId, frameRoutingId);
        expect(frame).not.to.be.null();
        expect(frame?.processId).to.be.equal(frameProcessId);
        expect(frame?.routingId).to.be.equal(frameRoutingId);
        expect(frame?.top === frame).to.be.equal(isMainFrame);
      }
    });
  });

  describe('"frame-created" event', () => {
    it('emits when the main frame is created', async () => {
      const w = new BrowserWindow({ show: false });
      const promise = emittedOnce(w.webContents, 'frame-created');
      w.webContents.loadFile(path.join(subframesPath, 'frame.html'));
      const [, details] = await promise;
      expect(details.frame).to.equal(w.webContents.mainFrame);
    });

    it('emits when nested frames are created', async () => {
      const w = new BrowserWindow({ show: false });
      const promise = emittedNTimes(w.webContents, 'frame-created', 2);
      w.webContents.loadFile(path.join(subframesPath, 'frame-container.html'));
      const [[, mainDetails], [, nestedDetails]] = await promise;
      expect(mainDetails.frame).to.equal(w.webContents.mainFrame);
      expect(nestedDetails.frame).to.equal(w.webContents.mainFrame.frames[0]);
    });

    it('is not emitted upon cross-origin navigation', async () => {
      const server = await createServer();

      // 黑客：使用“localhost”而不是“127.0.0.1”，因此Chromium将其视为。
      // 单独的来源，因为不同的端口没有足够的🤔。
      const secondUrl = `http:// 本地主机：${new url(server.url).port}`；

      const w = new BrowserWindow({ show: false });
      await w.webContents.loadURL(server.url);

      let frameCreatedEmitted = false;

      w.webContents.once('frame-created', () => {
        frameCreatedEmitted = true;
      });

      await w.webContents.loadURL(secondUrl);

      expect(frameCreatedEmitted).to.be.false();
    });
  });

  describe('"dom-ready" event', () => {
    it('emits for top-level frame', async () => {
      const w = new BrowserWindow({ show: false });
      const promise = emittedOnce(w.webContents.mainFrame, 'dom-ready');
      w.webContents.loadURL('about:blank');
      await promise;
    });

    it('emits for sub frame', async () => {
      const w = new BrowserWindow({ show: false });
      const promise = new Promise<void>(resolve => {
        w.webContents.on('frame-created', (e, { frame }) => {
          frame.on('dom-ready', () => {
            if (frame.name === 'frameA') {
              resolve();
            }
          });
        });
      });
      w.webContents.loadFile(path.join(subframesPath, 'frame-with-frame.html'));
      await promise;
    });
  });
});
