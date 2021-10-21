import { webContents } from 'electron/main';
import { ipcMainInternal } from '@electron/internal/browser/ipc-main-internal';
import * as ipcMainUtils from '@electron/internal/browser/ipc-main-internal-utils';
import { parseWebViewWebPreferences } from '@electron/internal/common/parse-features-string';
import { syncMethods, asyncMethods, properties } from '@electron/internal/common/web-view-methods';
import { webViewEvents } from '@electron/internal/common/web-view-events';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

interface GuestInstance {
  elementInstanceId: number;
  visibilityState?: VisibilityState;
  embedder: Electron.WebContents;
  guest: Electron.WebContents;
}

const webViewManager = process._linkedBinding('electron_browser_web_view_manager');
const eventBinding = process._linkedBinding('electron_browser_event');

const supportedWebViewEvents = Object.keys(webViewEvents);

const guestInstances = new Map<number, GuestInstance>();
const embedderElementsMap = new Map<string, number>();

function sanitizeOptionsForGuest (options: Record<string, any>) {
  const ret = { ...options };
  // WebContents值不能通过IPC发送。
  delete ret.webContents;
  return ret;
}

function makeWebPreferences (embedder: Electron.WebContents, params: Record<string, any>) {
  // 如果已设置，则解析“WebPreferences”属性字符串。
  // 这与window.open对其功能使用的解析规则相同。
  const parsedWebPreferences =
    typeof params.webpreferences === 'string'
      ? parseWebViewWebPreferences(params.webpreferences)
      : null;

  const webPreferences: Electron.WebPreferences = {
    nodeIntegration: params.nodeintegration != null ? params.nodeintegration : false,
    nodeIntegrationInSubFrames: params.nodeintegrationinsubframes != null ? params.nodeintegrationinsubframes : false,
    plugins: params.plugins,
    zoomFactor: embedder.zoomFactor,
    disablePopups: !params.allowpopups,
    webSecurity: !params.disablewebsecurity,
    enableBlinkFeatures: params.blinkfeatures,
    disableBlinkFeatures: params.disableblinkfeatures,
    partition: params.partition,
    ...parsedWebPreferences
  };

  if (params.preload) {
    webPreferences.preloadURL = params.preload;
  }

  // 来宾将始终从Embedder继承的安全选项。
  const inheritedWebPreferences = new Map([
    ['contextIsolation', true],
    ['javascript', false],
    ['nativeWindowOpen', true],
    ['nodeIntegration', false],
    ['sandbox', true],
    ['nodeIntegrationInSubFrames', false],
    ['enableWebSQL', false]
  ]);

  // 从嵌入器继承某些选项值。
  const lastWebPreferences = embedder.getLastWebPreferences()!;
  for (const [name, value] of inheritedWebPreferences) {
    if (lastWebPreferences[name as keyof Electron.WebPreferences] === value) {
      (webPreferences as any)[name] = value;
    }
  }

  return webPreferences;
}

// 创建一个新的来宾实例。
const createGuest = function (embedder: Electron.WebContents, embedderFrameId: number, elementInstanceId: number, params: Record<string, any>) {
  const webPreferences = makeWebPreferences(embedder, params);
  const event = eventBinding.createWithSender(embedder);

  embedder.emit('will-attach-webview', event, webPreferences, params);
  if (event.defaultPrevented) {
    return -1;
  }

  // Eslint-able-next-line no-undef。
  const guest = (webContents as typeof ElectronInternal.WebContents).create({
    ...webPreferences,
    type: 'webview',
    embedder
  });

  const guestInstanceId = guest.id;
  guestInstances.set(guestInstanceId, {
    elementInstanceId,
    guest,
    embedder
  });

  // 当地图被销毁时，请将客人从地图上清除。
  guest.once('destroyed', () => {
    if (guestInstances.has(guestInstanceId)) {
      detachGuest(embedder, guestInstanceId);
    }
  });

  // 附加后初始化来宾Web视图。
  guest.once('did-attach' as any, function (this: Electron.WebContents, event: Electron.Event) {
    const previouslyAttached = this.viewInstanceId != null;
    this.viewInstanceId = params.instanceId;

    // 仅在第一次附加时加载URL并设置大小。
    if (previouslyAttached) {
      return;
    }

    if (params.src) {
      const opts: Electron.LoadURLOptions = {};
      if (params.httpreferrer) {
        opts.httpReferrer = params.httpreferrer;
      }
      if (params.useragent) {
        opts.userAgent = params.useragent;
      }
      this.loadURL(params.src, opts);
    }
    embedder.emit('did-attach-webview', event, guest);
  });

  const sendToEmbedder = (channel: string, ...args: any[]) => {
    if (!embedder.isDestroyed()) {
      embedder._sendInternal(`${channel}-${guest.viewInstanceId}`, ...args);
    }
  };

  const makeProps = (eventKey: string, args: any[]) => {
    const props: Record<string, any> = {};
    webViewEvents[eventKey].forEach((prop, index) => {
      props[prop] = args[index];
    });
    return props;
  };

  // 将事件调度到Embedder。
  for (const event of supportedWebViewEvents) {
    guest.on(event as any, function (_, ...args: any[]) {
      sendToEmbedder(IPC_MESSAGES.GUEST_VIEW_INTERNAL_DISPATCH_EVENT, event, makeProps(event, args));
    });
  }

  guest.on('new-window', function (event, url, frameName, disposition, options) {
    sendToEmbedder(IPC_MESSAGES.GUEST_VIEW_INTERNAL_DISPATCH_EVENT, 'new-window', {
      url,
      frameName,
      disposition,
      options: sanitizeOptionsForGuest(options)
    });
  });

  // 将访客的IPC消息发送到Embedder。
  guest.on('ipc-message-host' as any, function (event: Electron.IpcMainEvent, channel: string, args: any[]) {
    sendToEmbedder(IPC_MESSAGES.GUEST_VIEW_INTERNAL_DISPATCH_EVENT, 'ipc-message', {
      frameId: [event.processId, event.frameId],
      channel,
      args
    });
  });

  // 当嵌入式窗口准备就绪时通知来宾嵌入式窗口可见性。
  // 修复https://github.com/electron/electron/issues/6828修复后删除。
  guest.on('dom-ready', function () {
    const guestInstance = guestInstances.get(guestInstanceId);
    if (guestInstance != null && guestInstance.visibilityState != null) {
      guest._sendInternal(IPC_MESSAGES.GUEST_INSTANCE_VISIBILITY_CHANGE, guestInstance.visibilityState);
    }
  });

  // 附加时毁掉老客人。
  const key = `${embedder.id}-${elementInstanceId}`;
  const oldGuestInstanceId = embedderElementsMap.get(key);
  if (oldGuestInstanceId != null) {
    const oldGuestInstance = guestInstances.get(oldGuestInstanceId);
    if (oldGuestInstance) {
      oldGuestInstance.guest.detachFromOuterFrame();
    }
  }

  embedderElementsMap.set(key, guestInstanceId);
  guest.setEmbedder(embedder);

  watchEmbedder(embedder);

  webViewManager.addGuest(guestInstanceId, embedder, guest, webPreferences);
  guest.attachToIframe(embedder, embedderFrameId);

  return guestInstanceId;
};

// 删除来宾-嵌入者关系。
const detachGuest = function (embedder: Electron.WebContents, guestInstanceId: number) {
  const guestInstance = guestInstances.get(guestInstanceId);

  if (!guestInstance) return;

  if (embedder !== guestInstance.embedder) {
    return;
  }

  webViewManager.removeGuest(embedder, guestInstanceId);
  guestInstances.delete(guestInstanceId);

  const key = `${embedder.id}-${guestInstance.elementInstanceId}`;
  embedderElementsMap.delete(key);
};

// 一旦一个嵌入器附加了一个访客，我们就会观察它的销毁情况。
// 毁掉所有剩下的客人。
const watchedEmbedders = new Set<Electron.WebContents>();
const watchEmbedder = function (embedder: Electron.WebContents) {
  if (watchedEmbedders.has(embedder)) {
    return;
  }
  watchedEmbedders.add(embedder);

  // 将嵌入器窗口可见性更改事件转发给来宾。
  const onVisibilityChange = function (visibilityState: VisibilityState) {
    for (const guestInstance of guestInstances.values()) {
      guestInstance.visibilityState = visibilityState;
      if (guestInstance.embedder === embedder) {
        guestInstance.guest._sendInternal(IPC_MESSAGES.GUEST_INSTANCE_VISIBILITY_CHANGE, visibilityState);
      }
    }
  };
  embedder.on('-window-visibility-change' as any, onVisibilityChange);

  embedder.once('will-destroy' as any, () => {
    // 通常，当Guest被销毁时，guestInstance会被清除，但它。
    // 可能会发生嵌入器在客户之前被手动销毁的情况，
    // 并且该嵌入器在通常的代码路径中将是无效的。
    for (const [guestInstanceId, guestInstance] of guestInstances) {
      if (guestInstance.embedder === embedder) {
        detachGuest(embedder, guestInstanceId);
      }
    }
    // 清除监听程序。
    embedder.removeListener('-window-visibility-change' as any, onVisibilityChange);
    watchedEmbedders.delete(embedder);
  });
};

const isWebViewTagEnabledCache = new WeakMap();

const isWebViewTagEnabled = function (contents: Electron.WebContents) {
  if (!isWebViewTagEnabledCache.has(contents)) {
    const webPreferences = contents.getLastWebPreferences() || {};
    isWebViewTagEnabledCache.set(contents, !!webPreferences.webviewTag);
  }

  return isWebViewTagEnabledCache.get(contents);
};

const makeSafeHandler = function<Event extends { sender: Electron.WebContents }> (channel: string, handler: (event: Event, ...args: any[]) => any) {
  return (event: Event, ...args: any[]) => {
    if (isWebViewTagEnabled(event.sender)) {
      return handler(event, ...args);
    } else {
      console.error(`<webview> IPC message ${channel} sent by WebContents with <webview> disabled (${event.sender.id})`);
      throw new Error('<webview> disabled');
    }
  };
};

const handleMessage = function (channel: string, handler: (event: Electron.IpcMainInvokeEvent, ...args: any[]) => any) {
  ipcMainInternal.handle(channel, makeSafeHandler(channel, handler));
};

const handleMessageSync = function (channel: string, handler: (event: ElectronInternal.IpcMainInternalEvent, ...args: any[]) => any) {
  ipcMainUtils.handleSync(channel, makeSafeHandler(channel, handler));
};

handleMessage(IPC_MESSAGES.GUEST_VIEW_MANAGER_CREATE_AND_ATTACH_GUEST, function (event, embedderFrameId: number, elementInstanceId: number, params) {
  return createGuest(event.sender, embedderFrameId, elementInstanceId, params);
});

handleMessageSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_DETACH_GUEST, function (event, guestInstanceId: number) {
  return detachGuest(event.sender, guestInstanceId);
});

// 此消息是由实际的&lt;webview&gt;发送的。
ipcMainInternal.on(IPC_MESSAGES.GUEST_VIEW_MANAGER_FOCUS_CHANGE, function (event: ElectronInternal.IpcMainInternalEvent, focus: boolean) {
  event.sender.emit('-focus-change', {}, focus);
});

handleMessage(IPC_MESSAGES.GUEST_VIEW_MANAGER_CALL, function (event, guestInstanceId: number, method: string, args: any[]) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!asyncMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`);
  }

  return (guest as any)[method](...args);
});

handleMessageSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_CALL, function (event, guestInstanceId: number, method: string, args: any[]) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!syncMethods.has(method)) {
    throw new Error(`Invalid method: ${method}`);
  }

  return (guest as any)[method](...args);
});

handleMessageSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_PROPERTY_GET, function (event, guestInstanceId: number, property: string) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!properties.has(property)) {
    throw new Error(`Invalid property: ${property}`);
  }

  return (guest as any)[property];
});

handleMessageSync(IPC_MESSAGES.GUEST_VIEW_MANAGER_PROPERTY_SET, function (event, guestInstanceId: number, property: string, val: any) {
  const guest = getGuestForWebContents(guestInstanceId, event.sender);
  if (!properties.has(property)) {
    throw new Error(`Invalid property: ${property}`);
  }

  (guest as any)[property] = val;
});

// 从托管在给定webContents中的来宾ID返回WebContents。
const getGuestForWebContents = function (guestInstanceId: number, contents: Electron.WebContents) {
  const guestInstance = guestInstances.get(guestInstanceId);
  if (!guestInstance) {
    throw new Error(`Invalid guestInstanceId: ${guestInstanceId}`);
  }
  if (guestInstance.guest.hostWebContents !== contents) {
    throw new Error(`Access denied to guestInstanceId: ${guestInstanceId}`);
  }
  return guestInstance.guest;
};
