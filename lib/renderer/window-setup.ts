import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { internalContextBridge } from '@electron/internal/renderer/api/context-bridge';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

const { contextIsolationEnabled } = internalContextBridge;

// 此文件通过CTX网桥实现以下API：
// -window.open()。
// -window.opener.blur()。
// -window.opener.close()。
// -window.opener.eval()。
// -window.opener.ocus()。
// -window.opener.location。
// -window.opener.print()。
// -window.opener.closed。
// -window.opener.postMessage()。
// -window.History y.back()。
// -window.History y.ward()。
// -window.History y.go()。
// -window.History y.length。
// -window.Prompt()。
// -文档。隐藏。
// -Docent.visibilityState。

// 解析相对url的帮助器函数。
const resolveURL = (url: string, base: string) => new URL(url, base).href;

// 使用此方法可确保主进程中预期的值为字符串。
// 在呈现器进程中可以转换为字符串。这确保了例外情况。
// 将值转换为字符串在此过程中抛出。
const toString = (value: any) => {
  return value != null ? `${value}` : value;
};

const windowProxies = new Map<number, BrowserWindowProxy>();

const getOrCreateProxy = (guestId: number): SafelyBoundBrowserWindowProxy => {
  let proxy = windowProxies.get(guestId);
  if (proxy == null) {
    proxy = new BrowserWindowProxy(guestId);
    windowProxies.set(guestId, proxy);
  }
  return proxy.getSafe();
};

const removeProxy = (guestId: number) => {
  windowProxies.delete(guestId);
};

type LocationProperties = 'hash' | 'href' | 'host' | 'hostname' | 'origin' | 'pathname' | 'port' | 'protocol' | 'search'

class LocationProxy {
  @LocationProxy.ProxyProperty public hash!: string;
  @LocationProxy.ProxyProperty public href!: string;
  @LocationProxy.ProxyProperty public host!: string;
  @LocationProxy.ProxyProperty public hostname!: string;
  @LocationProxy.ProxyProperty public origin!: string;
  @LocationProxy.ProxyProperty public pathname!: string;
  @LocationProxy.ProxyProperty public port!: string;
  @LocationProxy.ProxyProperty public protocol!: string;
  @LocationProxy.ProxyProperty public search!: URLSearchParams;

  private guestId: number;

  /* **注意：该修饰符将_Prototype_作为`target`。它定义了LocationProxy上的URL中常见的属性*。*/
  private static ProxyProperty<T> (target: LocationProxy, propertyKey: LocationProperties) {
    Object.defineProperty(target, propertyKey, {
      enumerable: true,
      configurable: true,
      get: function (this: LocationProxy): T | string {
        const guestURL = this.getGuestURL();
        const value = guestURL ? guestURL[propertyKey] : '';
        return value === undefined ? '' : value;
      },
      set: function (this: LocationProxy, newVal: T) {
        const guestURL = this.getGuestURL();
        if (guestURL) {
          // TypeScript不希望我们将其赋给只读变量。
          // 是的，那很糟糕，但我们还是要这么做。
          (guestURL as any)[propertyKey] = newVal;

          return this._invokeWebContentsMethod('loadURL', guestURL.toString());
        }
      }
    });
  }

  public getSafe = () => {
    const that = this;
    return {
      get href () { return that.href; },
      set href (newValue) { that.href = newValue; },
      get hash () { return that.hash; },
      set hash (newValue) { that.hash = newValue; },
      get host () { return that.host; },
      set host (newValue) { that.host = newValue; },
      get hostname () { return that.hostname; },
      set hostname (newValue) { that.hostname = newValue; },
      get origin () { return that.origin; },
      set origin (newValue) { that.origin = newValue; },
      get pathname () { return that.pathname; },
      set pathname (newValue) { that.pathname = newValue; },
      get port () { return that.port; },
      set port (newValue) { that.port = newValue; },
      get protocol () { return that.protocol; },
      set protocol (newValue) { that.protocol = newValue; },
      get search () { return that.search; },
      set search (newValue) { that.search = newValue; }
    };
  }

  constructor (guestId: number) {
    // Eslint会认为构造函数“毫无用处”
    // 除非我们把它们分配到身体里。很好，就是这样。
    // 不管怎样，这也行。
    this.guestId = guestId;
    this.getGuestURL = this.getGuestURL.bind(this);
  }

  public toString (): string {
    return this.href;
  }

  private getGuestURL (): URL | null {
    const maybeURL = this._invokeWebContentsMethodSync('getURL') as string;

    // 当没有之前的帧时，url将为空，因此请在此处说明这一点。
    // 以防止空字符串上的URL解析错误。
    const urlString = maybeURL !== '' ? maybeURL : 'about:blank';
    try {
      return new URL(urlString);
    } catch (e) {
      console.error('LocationProxy: failed to parse string', urlString, e);
    }

    return null;
  }

  private _invokeWebContentsMethod (method: string, ...args: any[]) {
    return ipcRendererInternal.invoke(IPC_MESSAGES.GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD, this.guestId, method, ...args);
  }

  private _invokeWebContentsMethodSync (method: string, ...args: any[]) {
    return ipcRendererUtils.invokeSync(IPC_MESSAGES.GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD, this.guestId, method, ...args);
  }
}

interface SafelyBoundBrowserWindowProxy {
  location: WindowProxy['location'];
  blur: WindowProxy['blur'];
  close: WindowProxy['close'];
  eval: typeof eval; // Eslint-Disable-line无求值。
  focus: WindowProxy['focus'];
  print: WindowProxy['print'];
  postMessage: WindowProxy['postMessage'];
  closed: boolean;
}

class BrowserWindowProxy {
  public closed: boolean = false

  private _location: LocationProxy
  private guestId: number

  // TypeScript不允许不同类型的getter/存取器，
  // 因此，目前，我们只能凑合着在组合中使用“Any”。
  // Https://github.com/Microsoft/TypeScript/issues/2521。
  public get location (): LocationProxy | any {
    return this._location.getSafe();
  }

  public set location (url: string | any) {
    url = resolveURL(url, this.location.href);
    this._invokeWebContentsMethod('loadURL', url);
  }

  constructor (guestId: number) {
    this.guestId = guestId;
    this._location = new LocationProxy(guestId);

    ipcRendererInternal.once(`${IPC_MESSAGES.GUEST_WINDOW_MANAGER_WINDOW_CLOSED}_${guestId}`, () => {
      removeProxy(guestId);
      this.closed = true;
    });
  }

  public getSafe = (): SafelyBoundBrowserWindowProxy => {
    const that = this;
    return {
      postMessage: this.postMessage,
      blur: this.blur,
      close: this.close,
      focus: this.focus,
      print: this.print,
      eval: this.eval,
      get location () {
        return that.location;
      },
      set location (url: string | any) {
        that.location = url;
      },
      get closed () {
        return that.closed;
      }
    };
  }

  public close = () => {
    this._invokeWindowMethod('destroy');
  }

  public focus = () => {
    this._invokeWindowMethod('focus');
  }

  public blur = () => {
    this._invokeWindowMethod('blur');
  }

  public print = () => {
    this._invokeWebContentsMethod('print');
  }

  public postMessage = (message: any, targetOrigin: string) => {
    ipcRendererInternal.invoke(IPC_MESSAGES.GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE, this.guestId, message, toString(targetOrigin), window.location.origin);
  }

  public eval = (code: string) => {
    this._invokeWebContentsMethod('executeJavaScript', code);
  }

  private _invokeWindowMethod (method: string, ...args: any[]) {
    return ipcRendererInternal.invoke(IPC_MESSAGES.GUEST_WINDOW_MANAGER_WINDOW_METHOD, this.guestId, method, ...args);
  }

  private _invokeWebContentsMethod (method: string, ...args: any[]) {
    return ipcRendererInternal.invoke(IPC_MESSAGES.GUEST_WINDOW_MANAGER_WEB_CONTENTS_METHOD, this.guestId, method, ...args);
  }
}

export const windowSetup = (
  isWebView: boolean, openerId: number, isHiddenPage: boolean, usesNativeWindowOpen: boolean) => {
  if (!process.sandboxed && !isWebView) {
    // 覆盖默认窗口。关闭。
    window.close = function () {
      ipcRendererInternal.send(IPC_MESSAGES.BROWSER_WINDOW_CLOSE);
    };
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueFromIsolatedWorld(['close'], window.close);
  }

  if (!usesNativeWindowOpen) {
    // TODO(MarshallOfSound)：兼容CTX隔离，无需打孔。
    // 使浏览器窗口或来宾视图发出“new-window”事件。
    window.open = function (url?: string, frameName?: string, features?: string) {
      if (url != null && url !== '') {
        url = resolveURL(url, location.href);
      }
      const guestId = ipcRendererInternal.sendSync(IPC_MESSAGES.GUEST_WINDOW_MANAGER_WINDOW_OPEN, url, toString(frameName), toString(features));
      if (guestId != null) {
        return getOrCreateProxy(guestId) as any as WindowProxy;
      } else {
        return null;
      }
    };
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueWithDynamicPropsFromIsolatedWorld(['open'], window.open);
  }

  // 如果此窗口使用nativeWindowOpen，但其打开窗口不使用，则我们。
  // 需要代理window.opener才能让页面与其。
  // 开场白。
  // 此外，从的nativeWindowOpen子级打开的窗口。
  // 非本机WindowOpen父级最初将拥有其WebPreferences。
  // 在更新之前从其打开程序复制，这意味着openerId。
  // 一开始是不正确的。我们通过检查。
  // Window.opener，它对于本地打开的子级来说将是非空的，因此我们。
  // 在这种情况下可以忽略openerId，因为它是从。
  // 家长。这个，呃，令人困惑，所以这张图可能会。
  // 帮助?。
  // 
  // [祖父窗口]--&gt;[父窗口]--&gt;[子窗口]。
  // N.W.O=FALSE N.W.O=TRUE N.W.O=TRUE。
  // Id=1 id=2 id=3。
  // OpenerId=0 openerId=1 openerId=1&lt;-！！错误！！
  // OPENER=NULL OPENER=NULL OPENER=[父窗口]。
  if (openerId && !window.opener) {
    window.opener = getOrCreateProxy(openerId);
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueWithDynamicPropsFromIsolatedWorld(['opener'], window.opener);
  }

  // 但是我们不支持Prompt()。
  window.prompt = function () {
    throw new Error('prompt() is and will not be supported.');
  };
  if (contextIsolationEnabled) internalContextBridge.overrideGlobalValueFromIsolatedWorld(['prompt'], window.prompt);

  if (!usesNativeWindowOpen || openerId) {
    ipcRendererInternal.on(IPC_MESSAGES.GUEST_WINDOW_POSTMESSAGE, function (
      _event, sourceId: number, message: any, sourceOrigin: string
    ) {
      // 手动调度事件而不是使用PostMessage，因为我们还需要。
      // 设置Event.Source。
      // 
      // 为什么会有呢？我们不能构造MessageEvent，也不能。
      // 使用`as MessageEvent`，因为您不应该重写。
      // 数据、来源和来源。
      const event: any = document.createEvent('Event');
      event.initEvent('message', false, false);

      event.data = message;
      event.origin = sourceOrigin;
      event.source = getOrCreateProxy(sourceId);

      window.dispatchEvent(event as MessageEvent);
    });
  }

  if (isWebView) {
    // WebView`Docent.visibilityState`跟踪窗口可见性(并忽略。
    // 实际的&lt;webview&gt;元素可见性)以实现向后兼容性。
    // 参见#9178中的讨论。
    // 
    // 请注意，这会导致重复的VisibilityChange事件(因为。
    // 铬也会激发它们)，并可能导致不正确的能见度变化。
    // 对于Electron2.0，我们应该重新考虑这一决定。
    let cachedVisibilityState = isHiddenPage ? 'hidden' : 'visible';

    // 订阅VisibilityState更改。
    ipcRendererInternal.on(IPC_MESSAGES.GUEST_INSTANCE_VISIBILITY_CHANGE, function (_event, visibilityState: VisibilityState) {
      if (cachedVisibilityState !== visibilityState) {
        cachedVisibilityState = visibilityState;
        document.dispatchEvent(new Event('visibilitychange'));
      }
    });

    // 使Docent.Hidden和Docent.visibilityState返回正确的值。
    const getDocumentHidden = () => cachedVisibilityState !== 'visible';
    Object.defineProperty(document, 'hidden', {
      get: getDocumentHidden
    });
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalPropertyFromIsolatedWorld(['document', 'hidden'], getDocumentHidden);

    const getDocumentVisibilityState = () => cachedVisibilityState;
    Object.defineProperty(document, 'visibilityState', {
      get: getDocumentVisibilityState
    });
    if (contextIsolationEnabled) internalContextBridge.overrideGlobalPropertyFromIsolatedWorld(['document', 'visibilityState'], getDocumentVisibilityState);
  }
};
