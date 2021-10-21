import { expect } from 'chai';
import * as http from 'http';
import * as qs from 'querystring';
import * as path from 'path';
import * as url from 'url';
import * as WebSocket from 'ws';
import { ipcMain, protocol, session, WebContents, webContents } from 'electron/main';
import { AddressInfo } from 'net';
import { emittedOnce } from './events-helpers';

const fixturesPath = path.resolve(__dirname, 'fixtures');

describe('webRequest module', () => {
  const ses = session.defaultSession;
  const server = http.createServer((req, res) => {
    if (req.url === '/serverRedirect') {
      res.statusCode = 301;
      res.setHeader('Location', 'http:// ‘+req.rawHeaders[1])；
      res.end();
    } else if (req.url === '/contentDisposition') {
      res.setHeader('content-disposition', [' attachment; filename=aa%E4%B8%ADaa.txt']);
      const content = req.url;
      res.end(content);
    } else {
      res.setHeader('Custom', ['Header']);
      let content = req.url;
      if (req.headers.accept === '*/* ；测试/头‘){Content+=’头/接收‘；}if(req.headers.Origin=’http://new-origin‘){Content+=’新/源‘；}res.end(Content)；}})；let defaultURL：String；之前((Done)=&gt;{Protocol.registerStringProtocol(’neworigin‘，(req，cb)=&gt;cb(’‘)；Server.listen(0，‘127.0.0.1’，()=&gt;{const port=(server.address()as AddressInfo).port；defaultURL=`http://127.0.0.1：${port}/`；Done()；})；After(()=&gt;{server.close()；protocol.unregisterProtocol(‘neworigin’)；})；let Contents：WebContents=NULL as WebContents；//nb。沙盒：使用True是因为它使导航速度快了很多(~8倍)。之前(async()=&gt;{Contents=(WebContents As Any).create({sandbox：true})；等待contents.loadFile(path.Join(fixturesPath，‘Pages’，‘jquery.html’))；})；After(()=&gt;(Contents As Any).销毁())；异步函数Ajax(url：string，options={}){return contents.ecuteJavaScript(`ajax(“${url}”，$。}Describe(‘webRequest.onBeforeRequest’，()=&gt;{AfterEach(()=&gt;{ses.webRequest.onBeforeRequest(Null)；})；it(‘可以取消请求’，async()=&gt;{ses.webRequest.onBeforeRequest((Details，expect(ajax(defaultURL)).to.eventually.be.rejectedWith(‘404’)；)=&gt;{Callback({Cancel：True})；})；等待回调。})；it(‘can filter urls’，async()=&gt;{const filter={urls：[defaultURL+‘filter/*’]}；ses.webRequest.onBeforeRequest(filter，(Details，callback)=&gt;{callback({ancel：true})；})；const{data}=等待AJAX(`${defaultURL}nofilter/test`)；expect(Data).to.equ.(‘/nofilter/。等待expect(ajax(`${defaultURL}filter/test`)).to.eventually.be.rejectedWith(‘404’)；})；it(‘Receiving Details Object’，Async()=&gt;{ses.webRequest.onBeforeRequest((Details，Callback)=&gt;{Expect(Details，Callback)=&gt;{Expect(Details.id).to.be.a(‘Number’)；Expect(Details.Timestamp).to.be.a(‘Number’)；Expect(details.webContentsId).to.be.a(‘number’)；expect(details.webContents).to.be.an(‘object’)；expect(details.webContents！.id).to.equal(details.webContentsId)；Expect(Details.Frame).to.be.an(‘Object’)；expect(details.url).to.be.a(‘string’).that.is.equal(defaultURL)；Expect(details.method).to.be.a(‘string’).that.is.equal(‘GET’)；expect(details.resourceType).to.be.a(‘string’).that.is.equal(‘xhr’)；Expect(Details.ploadData).to.be.unfined()；Callback({})；})；const{Data}=等待ajax(DefaultURL)；Expect(Data).to.equal(‘/’)；})；it(‘接收Details对象中的POST数据’，async()=&gt;{const postData={name：‘post test’，type：‘string’}；ses.webRequest.onBeforeRequest((Details，callback)=&gt;{Expect(Details.url).to.etc(DefaultURL)；Expect(Details.method).to.equale(‘post’)；Expect(details.uploadData).to.have.lengthOf(1)；常量数据=qs.parse(details.uploadData[0].bytes.toString())；Expect(Data).to.Deep.equence(PostData)；Cancel({Cancel：True})；})；等待预期(ajax(defaultURL，{type：‘post’，data：postData})).to.eventually.be.rejectedWith(‘404’)；})；it(‘可以重定向请求’，async()=&gt;{ses.webRequest.onBeforeRequest((Details，callback)=&gt;{if(Details.url=defaultURL){callback({redirectURL：`${defatURL。}Else{callback({})；}})；const{data}=等待ajax(DefaultURL)；expect(Data).to.equ.equence(‘/redirect’)；})；it(‘重定向不崩溃’，async()=&gt;{ses.webRequest.onBeforeRequest((Details，callback)=&gt;{callback({ancel：false})；})；等待ajax(defaultURL+‘serverRedirect’)；等待ajax(defaultURL+‘serverRedirect’)；})；it(‘works with file：//protocol’，async()=&gt;{ses.webRequest.onBeforeRequest((Details，callback)=&gt;{callback({ancel：true})；})；Const fileURL=url.format({路径名：path.Join(fixturespath，‘blank.html’).place(/\\/g，‘/’)，协议：‘file’，斜杠：TRUE})；等待expect(ajax(fileURL)).to.eventually.be.rejectedWith(‘404’)；})；})；Describe(‘webRequest.onBeforeSendHeaders’，()=&gt;{AfterEach(()=&gt;{ses.webRequest.onBeforeSendHeaders(Null)；ses.webRequest.onSendHeaders(Null)；})；it(‘Receives Details Object’，Async()=&gt;{ses.webRequest.onBeforeSendHeaders((Details，Callback)=&gt;{expect(details.requestHeaders).to.be.an(‘object’)；Expect(details.requestHeaders[‘Foo.Bar’]).to.equal(‘baz’)；回调({})；})；const{Data}=等待AJAX(defaultURL，{Headers：{‘Foo.Bar’：‘baz’*/*;test/header';
        callback({ requestHeaders: requestHeaders });
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/header/received');
    });

    it('can change the request headers on a custom protocol redirect', async () => {
      protocol.registerStringProtocol('custom-scheme', (req, callback) => {
        if (req.url === 'custom-scheme:// 
          callback({
            statusCode: 302,
            headers: {
              Location: 'custom-scheme:// 
            }
          });
        } else {
          let content = '';
          if (req.headers.Accept === '*/* */*;test/header';
          callback({ requestHeaders: requestHeaders });
        });
        const { data } = await ajax('custom-scheme:// 
        expect(data).to.equal('header-received');
      } finally {
        protocol.unregisterProtocol('custom-scheme');
      }
    });

    it('can change request origin', async () => {
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        const requestHeaders = details.requestHeaders;
        requestHeaders.Origin = 'http:// 
        callback({ requestHeaders: requestHeaders });
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/new/origin');
    });

    it('can capture CORS requests', async () => {
      let called = false;
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        called = true;
        callback({ requestHeaders: details.requestHeaders });
      });
      await ajax('neworigin:// 
      expect(called).to.be.true();
    });

    it('resets the whole headers', async () => {
      const requestHeaders = {
        Test: 'header'
      };
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        callback({ requestHeaders: requestHeaders });
      });
      ses.webRequest.onSendHeaders((details) => {
        expect(details.requestHeaders).to.deep.equal(requestHeaders);
      });
      await ajax(defaultURL);
    });

    it('leaves headers unchanged when no requestHeaders in callback', async () => {
      let originalRequestHeaders: Record<string, string>;
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        originalRequestHeaders = details.requestHeaders;
        callback({});
      });
      ses.webRequest.onSendHeaders((details) => {
        expect(details.requestHeaders).to.deep.equal(originalRequestHeaders);
      });
      await ajax(defaultURL);
    });

    it('works with file:// 
      const requestHeaders = {
        Test: 'header'
      };
      let onSendHeadersCalled = false;
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        callback({ requestHeaders: requestHeaders });
      });
      ses.webRequest.onSendHeaders((details) => {
        expect(details.requestHeaders).to.deep.equal(requestHeaders);
        onSendHeadersCalled = true;
      });
      await ajax(url.format({
        pathname: path.join(fixturesPath, 'blank.html').replace(/\\/g, '/'),
        protocol: 'file',
        slashes: true
      }));
      expect(onSendHeadersCalled).to.be.true();
    });
  });

  describe('webRequest.onSendHeaders', () => {
    afterEach(() => {
      ses.webRequest.onSendHeaders(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onSendHeaders((details) => {
        expect(details.requestHeaders).to.be.an('object');
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/');
    });
  });

  describe('webRequest.onHeadersReceived', () => {
    afterEach(() => {
      ses.webRequest.onHeadersReceived(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        expect(details.statusLine).to.equal('HTTP/1.1 200 OK');
        expect(details.statusCode).to.equal(200);
        expect(details.responseHeaders!.Custom).to.deep.equal(['Header']);
        callback({});
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/');
    });

    it('can change the response header', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders!;
        responseHeaders.Custom = ['Changed'] as any;
        callback({ responseHeaders: responseHeaders });
      });
      const { headers } = await ajax(defaultURL);
      expect(headers).to.match(/^custom: Changed$/m);
    });

    it('can change response origin', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders!;
        responseHeaders['access-control-allow-origin'] = ['http:// 
        callback({ responseHeaders: responseHeaders });
      });
      const { headers } = await ajax(defaultURL);
      expect(headers).to.match(/^access-control-allow-origin: http:\/\/new-origin$/m);
    });

    it('can change headers of CORS responses', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders!;
        responseHeaders.Custom = ['Changed'] as any;
        callback({ responseHeaders: responseHeaders });
      });
      const { headers } = await ajax('neworigin:// 
      expect(headers).to.match(/^custom: Changed$/m);
    });

    it('does not change header by default', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        callback({});
      });
      const { data, headers } = await ajax(defaultURL);
      expect(headers).to.match(/^custom: Header$/m);
      expect(data).to.equal('/');
    });

    it('does not change content-disposition header by default', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        expect(details.responseHeaders!['content-disposition']).to.deep.equal([' attachment; filename=aa中aa.txt']);
        callback({});
      });
      const { data, headers } = await ajax(defaultURL + 'contentDisposition');
      expect(headers).to.match(/^content-disposition: attachment; filename=aa%E4%B8%ADaa.txt$/m);
      expect(data).to.equal('/contentDisposition');
    });

    it('follows server redirect', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders;
        callback({ responseHeaders: responseHeaders });
      });
      const { headers } = await ajax(defaultURL + 'serverRedirect');
      expect(headers).to.match(/^custom: Header$/m);
    });

    it('can change the header status', async () => {
      ses.webRequest.onHeadersReceived((details, callback) => {
        const responseHeaders = details.responseHeaders;
        callback({
          responseHeaders: responseHeaders,
          statusLine: 'HTTP/1.1 404 Not Found'
        });
      });
      const { headers } = await contents.executeJavaScript(`new Promise((resolve, reject) => {
        const options = {
          ...${JSON.stringify({ url: defaultURL })},
          success: (data, status, request) => {
            reject(new Error('expected failure'))
          },
          error: (xhr) => {
            resolve({ headers: xhr.getAllResponseHeaders() })
          }
        }
        $.ajax(options)
      })`);
      expect(headers).to.match(/^custom: Header$/m);
    });
  });

  describe('webRequest.onResponseStarted', () => {
    afterEach(() => {
      ses.webRequest.onResponseStarted(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onResponseStarted((details) => {
        expect(details.fromCache).to.be.a('boolean');
        expect(details.statusLine).to.equal('HTTP/1.1 200 OK');
        expect(details.statusCode).to.equal(200);
        expect(details.responseHeaders!.Custom).to.deep.equal(['Header']);
      });
      const { data, headers } = await ajax(defaultURL);
      expect(headers).to.match(/^custom: Header$/m);
      expect(data).to.equal('/');
    });
  });

  describe('webRequest.onBeforeRedirect', () => {
    afterEach(() => {
      ses.webRequest.onBeforeRedirect(null);
      ses.webRequest.onBeforeRequest(null);
    });

    it('receives details object', async () => {
      const redirectURL = defaultURL + 'redirect';
      ses.webRequest.onBeforeRequest((details, callback) => {
        if (details.url === defaultURL) {
          callback({ redirectURL: redirectURL });
        } else {
          callback({});
        }
      });
      ses.webRequest.onBeforeRedirect((details) => {
        expect(details.fromCache).to.be.a('boolean');
        expect(details.statusLine).to.equal('HTTP/1.1 307 Internal Redirect');
        expect(details.statusCode).to.equal(307);
        expect(details.redirectURL).to.equal(redirectURL);
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/redirect');
    });
  });

  describe('webRequest.onCompleted', () => {
    afterEach(() => {
      ses.webRequest.onCompleted(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onCompleted((details) => {
        expect(details.fromCache).to.be.a('boolean');
        expect(details.statusLine).to.equal('HTTP/1.1 200 OK');
        expect(details.statusCode).to.equal(200);
      });
      const { data } = await ajax(defaultURL);
      expect(data).to.equal('/');
    });
  });

  describe('webRequest.onErrorOccurred', () => {
    afterEach(() => {
      ses.webRequest.onErrorOccurred(null);
      ses.webRequest.onBeforeRequest(null);
    });

    it('receives details object', async () => {
      ses.webRequest.onBeforeRequest((details, callback) => {
        callback({ cancel: true });
      });
      ses.webRequest.onErrorOccurred((details) => {
        expect(details.error).to.equal('net::ERR_BLOCKED_BY_CLIENT');
      });
      await expect(ajax(defaultURL)).to.eventually.be.rejectedWith('404');
    });
  });

  describe('WebSocket connections', () => {
    it('can be proxyed', async () => {
      // 
      const reqHeaders : { [key: string] : any } = {};
      const server = http.createServer((req, res) => {
        reqHeaders[req.url!] = req.headers;
        res.setHeader('foo1', 'bar1');
        res.end('ok');
      });
      const wss = new WebSocket.Server({ noServer: true });
      wss.on('connection', function connection (ws) {
        ws.on('message', function incoming (message) {
          if (message === 'foo') {
            ws.send('bar');
          }
        });
      });
      server.on('upgrade', function upgrade (request, socket, head) {
        const pathname = require('url').parse(request.url).pathname;
        if (pathname === '/websocket') {
          reqHeaders[request.url] = request.headers;
          wss.handleUpgrade(request, socket, head, function done (ws) {
            wss.emit('connection', ws, request);
          });
        }
      });

      // 
      await new Promise<void>(resolve => server.listen(0, '127.0.0.1', resolve));
      const port = String((server.address() as AddressInfo).port);

      // 
      const ses = session.fromPartition('WebRequestWebSocket');

      // 
      const receivedHeaders : { [key: string] : any } = {};
      ses.webRequest.onBeforeSendHeaders((details, callback) => {
        details.requestHeaders.foo = 'bar';
        callback({ requestHeaders: details.requestHeaders });
      });
      ses.webRequest.onHeadersReceived((details, callback) => {
        const pathname = require('url').parse(details.url).pathname;
        receivedHeaders[pathname] = details.responseHeaders;
        callback({ cancel: false });
      });
      ses.webRequest.onResponseStarted((details) => {
        if (details.url.startsWith('ws:// 
          expect(details.responseHeaders!.Connection[0]).be.equal('Upgrade');
        } else if (details.url.startsWith('http')) {
          expect(details.responseHeaders!.foo1[0]).be.equal('bar1');
        }
      });
      ses.webRequest.onSendHeaders((details) => {
        if (details.url.startsWith('ws:// 
          expect(details.requestHeaders.foo).be.equal('bar');
          expect(details.requestHeaders.Upgrade).be.equal('websocket');
        } else if (details.url.startsWith('http')) {
          expect(details.requestHeaders.foo).be.equal('bar');
        }
      });
      ses.webRequest.onCompleted((details) => {
        if (details.url.startsWith('ws:// 
          expect(details.error).be.equal('net::ERR_WS_UPGRADE');
        } else if (details.url.startsWith('http')) {
          expect(details.error).be.equal('net::OK');
        }
      });

      const contents = (webContents as any).create({
        session: ses,
        nodeIntegration: true,
        webSecurity: false,
        contextIsolation: false
      });

      // 
      after(() => {
        contents.destroy();
        server.close();
        ses.webRequest.onBeforeRequest(null);
        ses.webRequest.onBeforeSendHeaders(null);
        ses.webRequest.onHeadersReceived(null);
        ses.webRequest.onResponseStarted(null);
        ses.webRequest.onSendHeaders(null);
        ses.webRequest.onCompleted(null);
      });

      contents.loadFile(path.join(fixturesPath, 'api', 'webrequest.html'), { query: { port } });
      await emittedOnce(ipcMain, 'websocket-success');

      expect(receivedHeaders['/websocket'].Upgrade[0]).to.equal('websocket');
      expect(receivedHeaders['/'].foo1[0]).to.equal('bar1');
      expect(reqHeaders['/websocket'].foo).to.equal('bar');
      expect(reqHeaders['/'].foo).to.equal('bar');
    });
  });
});
