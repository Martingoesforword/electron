import type * as guestViewInternalModule from '@electron/internal/renderer/web-view/guest-view-internal';
import { WEB_VIEW_CONSTANTS } from '@electron/internal/renderer/web-view/web-view-constants';
import { syncMethods, asyncMethods, properties } from '@electron/internal/common/web-view-methods';
import type { WebViewAttribute, PartitionAttribute } from '@electron/internal/renderer/web-view/web-view-attributes';
import { setupWebViewAttributes } from '@electron/internal/renderer/web-view/web-view-attributes';

// 身份证生成器。
let nextId = 0;

const getNextId = function () {
  return ++nextId;
};

export interface WebViewImplHooks {
  readonly guestViewInternal: typeof guestViewInternalModule;
  readonly allowGuestViewElementDefinition: NodeJS.InternalWebFrame['allowGuestViewElementDefinition'];
  readonly setIsWebView: (iframe: HTMLIFrameElement) => void;
}

// 表示WebView节点的内部状态。
export class WebViewImpl {
  public beforeFirstNavigation = true
  public elementAttached = false
  public guestInstanceId?: number
  public hasFocus = false
  public internalInstanceId?: number;
  public resizeObserver?: ResizeObserver;
  public userAgentOverride?: string;
  public viewInstanceId: number

  // ON*事件处理程序。
  public on: Record<string, any> = {}
  public internalElement: HTMLIFrameElement

  public attributes: Map<string, WebViewAttribute>;

  constructor (public webviewNode: HTMLElement, private hooks: WebViewImplHooks) {
    // 创建内部IFRAME元素。
    this.internalElement = this.createInternalElement();
    const shadowRoot = this.webviewNode.attachShadow({ mode: 'open' });
    const style = shadowRoot.ownerDocument.createElement('style');
    style.textContent = ':host { display: flex; }';
    shadowRoot.appendChild(style);
    this.attributes = setupWebViewAttributes(this);
    this.viewInstanceId = getNextId();
    shadowRoot.appendChild(this.internalElement);

    // 提供对Content Window的访问。
    Object.defineProperty(this.webviewNode, 'contentWindow', {
      get: () => {
        return this.internalElement.contentWindow;
      },
      enumerable: true
    });
  }

  createInternalElement () {
    const iframeElement = document.createElement('iframe');
    iframeElement.style.flex = '1 1 auto';
    iframeElement.style.width = '100%';
    iframeElement.style.border = '0';
    // 由RendererClientBase：：IsWebViewFrame使用。
    this.hooks.setIsWebView(iframeElement);
    return iframeElement;
  }

  // 将&lt;webview&gt;元素重新附加到DOM时重置某些状态。
  reset () {
    // 如果定义了guestInstanceId，则&lt;webview&gt;已导航并具有。
    // 已经拾取了一个分区ID。因此，我们需要重置初始化。
    // 州政府。但是，可能出现的情况是beforeFirstNavigation值为假，但是。
    // GuestInstanceId尚未初始化。这意味着我们还没有。
    // 还没有收到CreateGuest的回复。在这种情况下，我们不会重置旗帜，因此。
    // 我们最终不会再安排第二位客人。
    if (this.guestInstanceId) {
      this.guestInstanceId = undefined;
    }

    this.beforeFirstNavigation = true;
    (this.attributes.get(WEB_VIEW_CONSTANTS.ATTRIBUTE_PARTITION) as PartitionAttribute).validPartitionId = true;

    // 由于附件将本地帧交换为远程帧，因此我们需要。
    // 将内部IFRAME元素重新设置为本地元素，然后才能重新附加。
    const newFrame = this.createInternalElement();
    const oldFrame = this.internalElement;
    this.internalElement = newFrame;

    if (oldFrame && oldFrame.parentNode) {
      oldFrame.parentNode.replaceChild(newFrame, oldFrame);
    }
  }

  // 此观察者监视&lt;webview&gt;和。
  // 相应地更新BrowserPlugin属性。反过来，更新。
  // BrowserPlugin属性将更新相应的BrowserPlugin。
  // 属性，如有必要。有关详细信息，请参阅BrowserPlugin：：UpdateDOMAttribute。
  // 细节。
  handleWebviewAttributeMutation (attributeName: string, oldValue: any, newValue: any) {
    if (!this.attributes.has(attributeName) || this.attributes.get(attributeName)!.ignoreMutation) {
      return;
    }

    // 让更改后的属性处理其自身的变化。
    this.attributes.get(attributeName)!.handleMutation(oldValue, newValue);
  }

  onElementResize () {
    const props = {
      newWidth: this.webviewNode.clientWidth,
      newHeight: this.webviewNode.clientHeight
    };
    this.dispatchEvent('resize', props);
  }

  createGuest () {
    this.internalInstanceId = getNextId();
    this.hooks.guestViewInternal.createGuest(this.internalElement, this.internalInstanceId, this.buildParams())
      .then(guestInstanceId => {
        this.attachGuestInstance(guestInstanceId);
      });
  }

  dispatchEvent (eventName: string, props: Record<string, any> = {}) {
    const event = new Event(eventName);
    Object.assign(event, props);
    this.webviewNode.dispatchEvent(event);

    if (eventName === 'load-commit') {
      this.onLoadCommit(props);
    } else if (eventName === '-focus-change') {
      this.onFocusChange();
    }
  }

  // 在Web视图上添加“on&lt;event&gt;”属性，该属性可用于设置/取消设置。
  // 事件处理程序。
  setupEventProperty (eventName: string) {
    const propertyName = `on${eventName.toLowerCase()}`;
    return Object.defineProperty(this.webviewNode, propertyName, {
      get: () => {
        return this.on[propertyName];
      },
      set: (value) => {
        if (this.on[propertyName]) {
          this.webviewNode.removeEventListener(eventName, this.on[propertyName]);
        }
        this.on[propertyName] = value;
        if (value) {
          return this.webviewNode.addEventListener(eventName, value);
        }
      },
      enumerable: true
    });
  }

  // 在加载提交时更新状态。
  onLoadCommit (props: Record<string, any>) {
    const oldValue = this.webviewNode.getAttribute(WEB_VIEW_CONSTANTS.ATTRIBUTE_SRC);
    const newValue = props.url;
    if (props.isMainFrame && (oldValue !== newValue)) {
      // 触摸src属性会触发导航。为了避免。
      // 在每个客户发起的导航上触发页面重新加载，
      // 我们不处理这种变异。
      this.attributes.get(WEB_VIEW_CONSTANTS.ATTRIBUTE_SRC)!.setValueIgnoreMutation(newValue);
    }
  }

  // 发射聚焦/模糊事件。
  onFocusChange () {
    const hasFocus = this.webviewNode.ownerDocument.activeElement === this.webviewNode;
    if (hasFocus !== this.hasFocus) {
      this.hasFocus = hasFocus;
      this.dispatchEvent(hasFocus ? 'focus' : 'blur');
    }
  }

  onAttach (storagePartitionId: number) {
    return this.attributes.get(WEB_VIEW_CONSTANTS.ATTRIBUTE_PARTITION)!.setValue(storagePartitionId);
  }

  buildParams () {
    const params: Record<string, any> = {
      instanceId: this.viewInstanceId,
      userAgentOverride: this.userAgentOverride
    };

    for (const [attributeName, attribute] of this.attributes) {
      params[attributeName] = attribute.getValue();
    }

    return params;
  }

  attachGuestInstance (guestInstanceId: number) {
    if (guestInstanceId === -1) {
      this.dispatchEvent('destroyed');
      return;
    }

    if (!this.elementAttached) {
      // 在我们从浏览器获得响应之前，元素可能会被分离。
      // 销毁支持的webContents以避免框架树中的任何僵尸节点。
      this.hooks.guestViewInternal.detachGuest(guestInstanceId);
      return;
    }

    this.guestInstanceId = guestInstanceId;
    // TODO(Zcbenz)：我们应该弃用“resize”事件吗？等等，这不是。
    // 甚至有记录在案。
    this.resizeObserver = new ResizeObserver(this.onElementResize.bind(this));
    this.resizeObserver.observe(this.internalElement);
  }
}

// 我希望埃斯林特不是那么愚蠢，但它确实是。
// Eslint-禁用-下一行。
export const setupMethods = (WebViewElement: typeof ElectronInternal.WebViewElement, hooks: WebViewImplHooks) => {
  // 聚焦Web视图应该会将页面焦点移到底层IFRAME。
  WebViewElement.prototype.focus = function () {
    this.contentWindow.focus();
  };

  // 将协议.foo*方法调用转发到WebViewImpl.foo*。
  for (const method of syncMethods) {
    (WebViewElement.prototype as Record<string, any>)[method] = function (this: ElectronInternal.WebViewElement, ...args: Array<any>) {
      return hooks.guestViewInternal.invokeSync(this.getWebContentsId(), method, args);
    };
  }

  for (const method of asyncMethods) {
    (WebViewElement.prototype as Record<string, any>)[method] = function (this: ElectronInternal.WebViewElement, ...args: Array<any>) {
      return hooks.guestViewInternal.invoke(this.getWebContentsId(), method, args);
    };
  }

  const createPropertyGetter = function (property: string) {
    return function (this: ElectronInternal.WebViewElement) {
      return hooks.guestViewInternal.propertyGet(this.getWebContentsId(), property);
    };
  };

  const createPropertySetter = function (property: string) {
    return function (this: ElectronInternal.WebViewElement, arg: any) {
      return hooks.guestViewInternal.propertySet(this.getWebContentsId(), property, arg);
    };
  };

  for (const property of properties) {
    Object.defineProperty(WebViewElement.prototype, property, {
      get: createPropertyGetter(property),
      set: createPropertySetter(property)
    });
  }
};
