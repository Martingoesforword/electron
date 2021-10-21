import * as url from 'url';
import { Readable, Writable } from 'stream';
import { app } from 'electron/main';
import type { ClientRequestConstructorOptions, UploadProgress } from 'electron/main';

const {
  isOnline,
  isValidHeaderName,
  isValidHeaderValue,
  createURLLoader
} = process._linkedBinding('electron_browser_net');

const kSupportedProtocols = new Set(['http:', 'https:']);

// Node.js为其丢弃重复项的标头集。
// 请参阅https://nodejs.org/api/http.html#http_message_headers。
const discardableDuplicateHeaders = new Set([
  'content-type',
  'content-length',
  'user-agent',
  'referer',
  'host',
  'authorization',
  'proxy-authorization',
  'if-modified-since',
  'if-unmodified-since',
  'from',
  'location',
  'max-forwards',
  'retry-after',
  'etag',
  'last-modified',
  'server',
  'age',
  'expires'
]);

class IncomingMessage extends Readable {
  _shouldPush: boolean;
  _data: (Buffer | null)[];
  _responseHead: NodeJS.ResponseHead;
  _resume: (() => void) | null;

  constructor (responseHead: NodeJS.ResponseHead) {
    super();
    this._shouldPush = false;
    this._data = [];
    this._responseHead = responseHead;
    this._resume = null;
  }

  get statusCode () {
    return this._responseHead.statusCode;
  }

  get statusMessage () {
    return this._responseHead.statusMessage;
  }

  get headers () {
    const filteredHeaders: Record<string, string | string[]> = {};
    const { rawHeaders } = this._responseHead;
    rawHeaders.forEach(header => {
      if (Object.prototype.hasOwnProperty.call(filteredHeaders, header.key) &&
          discardableDuplicateHeaders.has(header.key)) {
        // 不对可丢弃的重复标题执行任何操作。
      } else {
        if (header.key === 'set-cookie') {
          // 根据Node.js规则将set-cookie保留为数组。
          // 请参阅https://nodejs.org/api/http.html#http_message_headers。
          if (Object.prototype.hasOwnProperty.call(filteredHeaders, header.key)) {
            (filteredHeaders[header.key] as string[]).push(header.value);
          } else {
            filteredHeaders[header.key] = [header.value];
          }
        } else {
          // 对于非Cookie标头，这些值用‘，’连接在一起。
          if (Object.prototype.hasOwnProperty.call(filteredHeaders, header.key)) {
            filteredHeaders[header.key] += `, ${header.value}`;
          } else {
            filteredHeaders[header.key] = header.value;
          }
        }
      }
    });
    return filteredHeaders;
  }

  get httpVersion () {
    return `${this.httpVersionMajor}.${this.httpVersionMinor}`;
  }

  get httpVersionMajor () {
    return this._responseHead.httpVersion.major;
  }

  get httpVersionMinor () {
    return this._responseHead.httpVersion.minor;
  }

  get rawTrailers () {
    throw new Error('HTTP trailers are not supported');
  }

  get trailers () {
    throw new Error('HTTP trailers are not supported');
  }

  _storeInternalData (chunk: Buffer | null, resume: (() => void) | null) {
    // 保存网络回调以供in_ush InternalData使用。
    this._resume = resume;
    this._data.push(chunk);
    this._pushInternalData();
  }

  _pushInternalData () {
    while (this._shouldPush && this._data.length > 0) {
      const chunk = this._data.shift();
      this._shouldPush = this.push(chunk);
    }
    if (this._shouldPush && this._resume) {
      // 重置回调，以便每个回调都使用一个新的回调。
      // 一批节流数据。在调用Resume之前执行此操作，以避免出现。
      // 潜在的竞争条件。
      const resume = this._resume;
      this._resume = null;

      resume();
    }
  }

  _read () {
    this._shouldPush = true;
    this._pushInternalData();
  }
}

/* *缓冲写入其中的所有内容的可写流。*/
class SlurpStream extends Writable {
  _data: Buffer;
  constructor () {
    super();
    this._data = Buffer.alloc(0);
  }

  _write (chunk: Buffer, encoding: string, callback: () => void) {
    this._data = Buffer.concat([this._data, chunk]);
    callback();
  }

  data () { return this._data; }
}

class ChunkedBodyStream extends Writable {
  _pendingChunk: Buffer | undefined;
  _downstream?: NodeJS.DataPipe;
  _pendingCallback?: (error?: Error) => void;
  _clientRequest: ClientRequest;

  constructor (clientRequest: ClientRequest) {
    super();
    this._clientRequest = clientRequest;
  }

  _write (chunk: Buffer, encoding: string, callback: () => void) {
    if (this._downstream) {
      this._downstream.write(chunk).then(callback, callback);
    } else {
      // _WRITE的约定是，我们不会再被调用，直到我们调用。
      // 回调，所以我们只保存一块就行了。
      this._pendingChunk = chunk;
      this._pendingCallback = callback;

      // 对分块的正文流的第一次写入将开始请求。
      this._clientRequest._startRequest();
    }
  }

  _final (callback: () => void) {
    this._downstream!.done();
    callback();
  }

  startReading (pipe: NodeJS.DataPipe) {
    if (this._downstream) {
      throw new Error('two startReading calls???');
    }
    this._downstream = pipe;
    if (this._pendingChunk) {
      const doneWriting = (maybeError: Error | void) => {
        // 如果底层请求已中止，我们不关心错误。
        // 无论如何，一旦我们中止，所有工作都应该立即停止，此错误可能是。
        // “MOJO PIPE DIRECTED”错误(代码=9)。
        if (this._clientRequest._aborted) return;

        const cb = this._pendingCallback!;
        delete this._pendingCallback;
        delete this._pendingChunk;
        cb(maybeError || undefined);
      };
      this._downstream.write(this._pendingChunk).then(doneWriting, doneWriting);
    }
  }
}

type RedirectPolicy = 'manual' | 'follow' | 'error';

function parseOptions (optionsIn: ClientRequestConstructorOptions | string): NodeJS.CreateURLLoaderOptions & { redirectPolicy: RedirectPolicy, headers: Record<string, { name: string, value: string | string[] }> } {
  const options: any = typeof optionsIn === 'string' ? url.parse(optionsIn) : { ...optionsIn };

  let urlStr: string = options.url;

  if (!urlStr) {
    const urlObj: url.UrlObject = {};
    const protocol = options.protocol || 'http:';
    if (!kSupportedProtocols.has(protocol)) {
      throw new Error('Protocol "' + protocol + '" not supported');
    }
    urlObj.protocol = protocol;

    if (options.host) {
      urlObj.host = options.host;
    } else {
      if (options.hostname) {
        urlObj.hostname = options.hostname;
      } else {
        urlObj.hostname = 'localhost';
      }

      if (options.port) {
        urlObj.port = options.port;
      }
    }

    if (options.path && / /.test(options.path)) {
      // 实际的正则表达式更像/[^A-ZA-Z0-9\-._~！$&‘()*+，；=/：@]/。
      // 带有用于忽略百分比转义字符的附加规则。
      // 但这是a)很难在执行的正则表达式中捕获。
      // 嗯，和b)可能对现实世界的使用限制太多。那是。
      // 为什么它只扫描空间，因为这些空间保证会创建。
      // 无效的请求。
      throw new TypeError('Request path contains unescaped characters');
    }
    const pathObj = url.parse(options.path || '/');
    urlObj.pathname = pathObj.pathname;
    urlObj.search = pathObj.search;
    urlObj.hash = pathObj.hash;
    urlStr = url.format(urlObj);
  }

  const redirectPolicy = options.redirect || 'follow';
  if (!['follow', 'error', 'manual'].includes(redirectPolicy)) {
    throw new Error('redirect mode should be one of follow, error or manual');
  }

  if (options.headers != null && typeof options.headers !== 'object') {
    throw new TypeError('headers must be an object');
  }

  const urlLoaderOptions: NodeJS.CreateURLLoaderOptions & { redirectPolicy: RedirectPolicy, headers: Record<string, { name: string, value: string | string[] }> } = {
    method: (options.method || 'GET').toUpperCase(),
    url: urlStr,
    redirectPolicy,
    headers: {},
    body: null as any,
    useSessionCookies: options.useSessionCookies,
    credentials: options.credentials,
    origin: options.origin
  };
  const headers: Record<string, string | string[]> = options.headers || {};
  for (const [name, value] of Object.entries(headers)) {
    if (!isValidHeaderName(name)) {
      throw new Error(`Invalid header name: '${name}'`);
    }
    if (!isValidHeaderValue(value.toString())) {
      throw new Error(`Invalid value for header '${name}': '${value}'`);
    }
    const key = name.toLowerCase();
    urlLoaderOptions.headers[key] = { name, value };
  }
  if (options.session) {
    // 检查不力，但应该足以捕捉到99%的意外误用。
    if (options.session.constructor && options.session.constructor.name === 'Session') {
      urlLoaderOptions.session = options.session;
    } else {
      throw new TypeError('`session` should be an instance of the Session class');
    }
  } else if (options.partition) {
    if (typeof options.partition === 'string') {
      urlLoaderOptions.partition = options.partition;
    } else {
      throw new TypeError('`partition` should be a string');
    }
  }
  return urlLoaderOptions;
}

export class ClientRequest extends Writable implements Electron.ClientRequest {
  _started: boolean = false;
  _firstWrite: boolean = false;
  _aborted: boolean = false;
  _chunkedEncoding: boolean | undefined;
  _body: Writable | undefined;
  _urlLoaderOptions: NodeJS.CreateURLLoaderOptions & { headers: Record<string, { name: string, value: string | string[] }> };
  _redirectPolicy: RedirectPolicy;
  _followRedirectCb?: () => void;
  _uploadProgress?: { active: boolean, started: boolean, current: number, total: number };
  _urlLoader?: NodeJS.URLLoader;
  _response?: IncomingMessage;

  constructor (options: ClientRequestConstructorOptions | string, callback?: (message: IncomingMessage) => void) {
    super({ autoDestroy: true });

    if (!app.isReady()) {
      throw new Error('net module can only be used after app is ready');
    }

    if (callback) {
      this.once('response', callback);
    }

    const { redirectPolicy, ...urlLoaderOptions } = parseOptions(options);
    this._urlLoaderOptions = urlLoaderOptions;
    this._redirectPolicy = redirectPolicy;
  }

  get chunkedEncoding () {
    return this._chunkedEncoding || false;
  }

  set chunkedEncoding (value: boolean) {
    if (this._started) {
      throw new Error('chunkedEncoding can only be set before the request is started');
    }
    if (typeof this._chunkedEncoding !== 'undefined') {
      throw new Error('chunkedEncoding can only be set once');
    }
    this._chunkedEncoding = !!value;
    if (this._chunkedEncoding) {
      this._body = new ChunkedBodyStream(this);
      this._urlLoaderOptions.body = (pipe: NodeJS.DataPipe) => {
        (this._body! as ChunkedBodyStream).startReading(pipe);
      };
    }
  }

  setHeader (name: string, value: string) {
    if (typeof name !== 'string') {
      throw new TypeError('`name` should be a string in setHeader(name, value)');
    }
    if (value == null) {
      throw new Error('`value` required in setHeader("' + name + '", value)');
    }
    if (this._started || this._firstWrite) {
      throw new Error('Can\'t set headers after they are sent');
    }
    if (!isValidHeaderName(name)) {
      throw new Error(`Invalid header name: '${name}'`);
    }
    if (!isValidHeaderValue(value.toString())) {
      throw new Error(`Invalid value for header '${name}': '${value}'`);
    }

    const key = name.toLowerCase();
    this._urlLoaderOptions.headers[key] = { name, value };
  }

  getHeader (name: string) {
    if (name == null) {
      throw new Error('`name` is required for getHeader(name)');
    }

    const key = name.toLowerCase();
    const header = this._urlLoaderOptions.headers[key];
    return header && header.value as any;
  }

  removeHeader (name: string) {
    if (name == null) {
      throw new Error('`name` is required for removeHeader(name)');
    }

    if (this._started || this._firstWrite) {
      throw new Error('Can\'t remove headers after they are sent');
    }

    const key = name.toLowerCase();
    delete this._urlLoaderOptions.headers[key];
  }

  _write (chunk: Buffer, encoding: BufferEncoding, callback: () => void) {
    this._firstWrite = true;
    if (!this._body) {
      this._body = new SlurpStream();
      this._body.on('finish', () => {
        this._urlLoaderOptions.body = (this._body as SlurpStream).data();
        this._startRequest();
      });
    }
    // TODO：这是转发到另一个流的正确方式吗？
    this._body.write(chunk, encoding, callback);
  }

  _final (callback: () => void) {
    if (this._body) {
      // TODO：这是转发到另一个流的正确方式吗？
      this._body.end(callback);
    } else {
      // 在没有正文的情况下调用end()，请继续并启动请求。
      this._startRequest();
      callback();
    }
  }

  _startRequest () {
    this._started = true;
    const stringifyValues = (obj: Record<string, { name: string, value: string | string[] }>) => {
      const ret: Record<string, string> = {};
      for (const k of Object.keys(obj)) {
        const kv = obj[k];
        ret[kv.name] = kv.value.toString();
      }
      return ret;
    };
    this._urlLoaderOptions.referrer = this.getHeader('referer') || '';
    this._urlLoaderOptions.origin = this._urlLoaderOptions.origin || this.getHeader('origin') || '';
    this._urlLoaderOptions.hasUserActivation = this.getHeader('sec-fetch-user') === '?1';
    this._urlLoaderOptions.mode = this.getHeader('sec-fetch-mode') || '';
    this._urlLoaderOptions.destination = this.getHeader('sec-fetch-dest') || '';
    const opts = { ...this._urlLoaderOptions, extraHeaders: stringifyValues(this._urlLoaderOptions.headers) };
    this._urlLoader = createURLLoader(opts);
    this._urlLoader.on('response-started', (event, finalUrl, responseHead) => {
      const response = this._response = new IncomingMessage(responseHead);
      this.emit('response', response);
    });
    this._urlLoader.on('data', (event, data, resume) => {
      this._response!._storeInternalData(Buffer.from(data), resume);
    });
    this._urlLoader.on('complete', () => {
      if (this._response) { this._response._storeInternalData(null, null); }
    });
    this._urlLoader.on('error', (event, netErrorString) => {
      const error = new Error(netErrorString);
      if (this._response) this._response.destroy(error);
      this._die(error);
    });

    this._urlLoader.on('login', (event, authInfo, callback) => {
      const handled = this.emit('login', authInfo, callback);
      if (!handled) {
        // 如果没有监听程序，则取消身份验证请求。
        callback();
      }
    });

    this._urlLoader.on('redirect', (event, redirectInfo, headers) => {
      const { statusCode, newMethod, newUrl } = redirectInfo;
      if (this._redirectPolicy === 'error') {
        this._die(new Error('Attempted to redirect, but redirect policy was \'error\''));
      } else if (this._redirectPolicy === 'manual') {
        let _followRedirect = false;
        this._followRedirectCb = () => { _followRedirect = true; };
        try {
          this.emit('redirect', statusCode, newMethod, newUrl, headers);
        } finally {
          this._followRedirectCb = undefined;
          if (!_followRedirect && !this._aborted) {
            this._die(new Error('Redirect was cancelled'));
          }
        }
      } else if (this._redirectPolicy === 'follow') {
        // 当重定向策略为‘Follow’时调用postRedirect()是。
        // 允许，但不执行任何操作。(也许它应该抛出一个错误。
        // 不过...？因为不管怎样，重定向都会发生。)。
        try {
          this._followRedirectCb = () => {};
          this.emit('redirect', statusCode, newMethod, newUrl, headers);
        } finally {
          this._followRedirectCb = undefined;
        }
      } else {
        this._die(new Error(`Unexpected redirect policy '${this._redirectPolicy}'`));
      }
    });

    this._urlLoader.on('upload-progress', (event, position, total) => {
      this._uploadProgress = { active: true, started: true, current: position, total };
      this.emit('upload-progress', position, total); // 目前，无证可查。
    });

    this._urlLoader.on('download-progress', (event, current) => {
      if (this._response) {
        this._response.emit('download-progress', current); // 目前，无证可查。
      }
    });
  }

  followRedirect () {
    if (this._followRedirectCb) {
      this._followRedirectCb();
    } else {
      throw new Error('followRedirect() called, but was not waiting for a redirect');
    }
  }

  abort () {
    if (!this._aborted) {
      process.nextTick(() => { this.emit('abort'); });
    }
    this._aborted = true;
    this._die();
  }

  _die (err?: Error) {
    // Js假定结束的任何流不再能够发出事件。
    // 对于行为类似于流的对象的情况，这是一个错误的假设。
    // (我们的urlRequest)。如果我们不在这里发出，这会导致错误，因为我们“确实”期望。
    // 该错误事件可以在urlRequest.end()之后发出。
    if ((this as any)._writableState.destroyed && err) {
      this.emit('error', err);
    }

    this.destroy(err);
    if (this._urlLoader) {
      this._urlLoader.cancel();
      if (this._response) this._response.destroy(err);
    }
  }

  getUploadProgress (): UploadProgress {
    return this._uploadProgress ? { ...this._uploadProgress } : { active: false, started: false, current: 0, total: 0 };
  }
}

export function request (options: ClientRequestConstructorOptions | string, callback?: (message: IncomingMessage) => void) {
  return new ClientRequest(options, callback);
}

exports.isOnline = isOnline;

Object.defineProperty(exports, 'online', {
  get: () => isOnline()
});
