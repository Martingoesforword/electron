/* **创建并最小限度地跟踪渲染器方向的来宾窗口*(通过window.open)。在这里，“Guest”大致表示“Child”--它不一定*象征着它的进程状态；进程内(同源*nativeWindowOpen)和进程外(跨源nativeWindowOpen和*BrowserWindowProxy)都是在这里创建的。“Embedder”大致意思是“父母”*/
import { BrowserWindow } from 'electron/main';
import type { BrowserWindowConstructorOptions, Referrer, WebContents, LoadURLOptions } from 'electron/main';
import { parseFeatures } from '@electron/internal/common/parse-features-string';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

type PostData = LoadURLOptions['postData']
export type WindowOpenArgs = {
  url: string,
  frameName: string,
  features: string,
}

const frameNamesToWindow = new Map<string, BrowserWindow>();
const registerFrameNameToGuestWindow = (name: string, win: BrowserWindow) => frameNamesToWindow.set(name, win);
const unregisterFrameName = (name: string) => frameNamesToWindow.delete(name);
const getGuestWindowByFrameName = (name: string) => frameNamesToWindow.get(name);

/* Window.open*(BrowserWindowProxy和nativeWindowOpen)的两种实现都会调用**`openGuestWindow`来创建和设置新窗口的事件处理*。**在12.0.0中移除`new-window`事件之前，会触发`new-window`事件，允许*用户在传递的事件上预防Default()(最终在nativeWindowOpen代码路径中调用*DestroyWebContents)。*/
export function openGuestWindow ({ event, embedder, guest, referrer, disposition, postData, overrideBrowserWindowOptions, windowOpenArgs }: {
  event: { sender: WebContents, defaultPrevented: boolean },
  embedder: WebContents,
  guest?: WebContents,
  referrer: Referrer,
  disposition: string,
  postData?: PostData,
  overrideBrowserWindowOptions?: BrowserWindowConstructorOptions,
  windowOpenArgs: WindowOpenArgs,
}): BrowserWindow | undefined {
  const { url, frameName, features } = windowOpenArgs;
  const { options: browserWindowOptions } = makeBrowserWindowOptions({
    embedder,
    features,
    overrideOptions: overrideBrowserWindowOptions
  });

  const didCancelEvent = emitDeprecatedNewWindowEvent({
    event,
    embedder,
    guest,
    browserWindowOptions,
    windowOpenArgs,
    disposition,
    postData,
    referrer
  });
  if (didCancelEvent) return;

  // 要进行规范，后续的window.open调用使用相同的帧名(中的`target`。
  // 规范术语)将重用以前的窗口。
  // Https://html.spec.whatwg.org/multipage/window-object.html#apis-for-creating-and-navigating-browsing-contexts-by-name。
  const existingWindow = getGuestWindowByFrameName(frameName);
  if (existingWindow) {
    if (existingWindow.isDestroyed() || existingWindow.webContents.isDestroyed()) {
      // FIXME(T57ser)：WebContents由于某种原因被销毁，请注销框架名称。
      unregisterFrameName(frameName);
    } else {
      existingWindow.loadURL(url);
      return existingWindow;
    }
  }

  const window = new BrowserWindow({
    webContents: guest,
    ...browserWindowOptions
  });
  if (!guest) {
    // 如果webContents是由我们在。
    // BrowserWindowProxy(非沙盒，nativeWindowOpen：false)的情况，
    // 对象创建窗口时导航到url。
    // WebContents不是必需的(无论如何它都会导航到那里)。
    // 如果我们从OpenURLFromTab中输入此函数，也可能会发生这种情况。
    // 在哪种情况下，浏览器进程负责启动导航。
    // 在新窗口中。
    window.loadURL(url, {
      httpReferrer: referrer,
      ...(postData && {
        postData,
        extraHeaders: formatPostDataHeaders(postData as Electron.UploadRawData[])
      })
    });
  }

  handleWindowLifecycleEvents({ embedder, frameName, guest: window });

  embedder.emit('did-create-window', window, { url, frameName, options: browserWindowOptions, disposition, referrer, postData });

  return window;
}

/* **管理嵌入器窗口和访客窗口的关系。当*Guest被销毁时，通知嵌入器。当嵌入器被销毁时，*访客也会被销毁；这是电子惯例，而不是基于*浏览器行为。*/
const handleWindowLifecycleEvents = function ({ embedder, guest, frameName }: {
  embedder: WebContents,
  guest: BrowserWindow,
  frameName: string
}) {
  const closedByEmbedder = function () {
    guest.removeListener('closed', closedByUser);
    guest.destroy();
  };

  const cachedGuestId = guest.webContents.id;
  const closedByUser = function () {
    embedder._sendInternal(`${IPC_MESSAGES.GUEST_WINDOW_MANAGER_WINDOW_CLOSED}_${cachedGuestId}`);
    embedder.removeListener('current-render-view-deleted' as any, closedByEmbedder);
  };
  embedder.once('current-render-view-deleted' as any, closedByEmbedder);
  guest.once('closed', closedByUser);

  if (frameName) {
    registerFrameNameToGuestWindow(frameName, guest);
    guest.once('closed', function () {
      unregisterFrameName(frameName);
    });
  }
};

/* **11.0.0版本中支持`webContents.setWindowOpenHandler`和*`had-create-window`。将在12.0.0中删除。*/
function emitDeprecatedNewWindowEvent ({ event, embedder, guest, windowOpenArgs, browserWindowOptions, disposition, referrer, postData }: {
  event: { sender: WebContents, defaultPrevented: boolean, newGuest?: BrowserWindow },
  embedder: WebContents,
  guest?: WebContents,
  windowOpenArgs: WindowOpenArgs,
  browserWindowOptions: BrowserWindowConstructorOptions,
  disposition: string,
  referrer: Referrer,
  postData?: PostData,
}): boolean {
  const { url, frameName } = windowOpenArgs;
  const isWebViewWithPopupsDisabled = embedder.getType() === 'webview' && embedder.getLastWebPreferences()!.disablePopups;
  const postBody = postData ? {
    data: postData,
    ...parseContentTypeFormat(postData)
  } : null;

  embedder.emit(
    'new-window',
    event,
    url,
    frameName,
    disposition,
    {
      ...browserWindowOptions,
      webContents: guest
    },
    [], // 附加功能。
    referrer,
    postBody
  );

  const { newGuest } = event;
  if (isWebViewWithPopupsDisabled) return true;
  if (event.defaultPrevented) {
    if (newGuest) {
      if (guest === newGuest.webContents) {
        // 未更改webContents，因此将defaultPrevent设置为false。
        // 阻止此事件的调用方销毁webContents。
        event.defaultPrevented = false;
      }

      handleWindowLifecycleEvents({
        embedder: event.sender,
        guest: newGuest,
        frameName
      });
    }
    return true;
  }
  return false;
}

// 子窗口将始终从父窗口继承的安全选项。
const securityWebPreferences: { [key: string]: boolean } = {
  contextIsolation: true,
  javascript: false,
  nativeWindowOpen: true,
  nodeIntegration: false,
  sandbox: true,
  webviewTag: false,
  nodeIntegrationInSubFrames: false,
  enableWebSQL: false
};

function makeBrowserWindowOptions ({ embedder, features, overrideOptions }: {
  embedder: WebContents,
  features: string,
  overrideOptions?: BrowserWindowConstructorOptions,
}) {
  const { options: parsedOptions, webPreferences: parsedWebPreferences } = parseFeatures(features);

  return {
    options: {
      show: true,
      width: 800,
      height: 600,
      ...parsedOptions,
      ...overrideOptions,
      webPreferences: makeWebPreferences({
        embedder,
        insecureParsedWebPreferences: parsedWebPreferences,
        secureOverrideWebPreferences: overrideOptions && overrideOptions.webPreferences
      })
    } as Electron.BrowserViewConstructorOptions
  };
}

export function makeWebPreferences ({ embedder, secureOverrideWebPreferences = {}, insecureParsedWebPreferences: parsedWebPreferences = {} }: {
  embedder: WebContents,
  insecureParsedWebPreferences?: ReturnType<typeof parseFeatures>['webPreferences'],
  // 请注意，覆盖首选项被认为是提升的，并且仅应。
  // 源自主进程，因为它们覆盖了安全默认值。如果你。
  // 如果有未经审查的首选项，请使用parsedWebPreferences。
  secureOverrideWebPreferences?: BrowserWindowConstructorOptions['webPreferences'],
}) {
  const parentWebPreferences = embedder.getLastWebPreferences()!;
  const securityWebPreferencesFromParent = (Object.keys(securityWebPreferences).reduce((map, key) => {
    if (securityWebPreferences[key] === parentWebPreferences[key as keyof Electron.WebPreferences]) {
      (map as any)[key] = parentWebPreferences[key as keyof Electron.WebPreferences];
    }
    return map;
  }, {} as Electron.WebPreferences));
  const openerId = parentWebPreferences.nativeWindowOpen ? null : embedder.id;

  return {
    ...parsedWebPreferences,
    // 请注意，顺序是这里的关键，我们想要禁止渲染器的。
    // 能够更改重要的安全选项，但允许Main(通过。
    // SetWindowOpenHandler)来更改它们。
    ...securityWebPreferencesFromParent,
    ...secureOverrideWebPreferences,
    // 在此处设置正确的openerId，以便为‘new-window’事件处理程序提供正确的选项。
    // TODO：想出另一种方法来通过这件事？
    openerId
  };
}

function formatPostDataHeaders (postData: PostData) {
  if (!postData) return;

  const { contentType, boundary } = parseContentTypeFormat(postData);
  if (boundary != null) { return `content-type: ${contentType}; boundary=${boundary}`; }

  return `content-type: ${contentType}`;
}

const MULTIPART_CONTENT_TYPE = 'multipart/form-data';
const URL_ENCODED_CONTENT_TYPE = 'application/x-www-form-urlencoded';

// 找出发布数据的适当标题。
export const parseContentTypeFormat = function (postData: Exclude<PostData, undefined>) {
  if (postData.length) {
    if (postData[0].type === 'rawData') {
      // 对于多部分表单，第一个元素将从边界开始。
      // 注意，它类似于`-WebKitFormBorary12345678`。
      // 注意，此正则表达式在提交具有。
      // 输入属性name=“--thekey”，但是，呃，不这样做吗？
      const postDataFront = postData[0].bytes.toString();
      const boundary = /^--.*[^-\r\n]/.exec(postDataFront);
      if (boundary) {
        return {
          boundary: boundary[0].substr(2),
          contentType: MULTIPART_CONTENT_TYPE
        };
      }
    }
  }
  // 表单提交不包含任何输入(postData数组。
  // 是空的)，或者我们找不到边界，因此我们可以假设这是。
  // A键=值样式表单。
  return {
    contentType: URL_ENCODED_CONTENT_TYPE
  };
};
