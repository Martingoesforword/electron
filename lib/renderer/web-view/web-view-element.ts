// 使用上下文隔离时，WebViewElement和自定义元素。
// 方法必须在主世界中定义才能注册。
// 
// 注意：隐藏的值只能在相同的上下文中读取/设置，所有。
// 访问“内部”隐藏值的方法必须放入此文件中。
// 
// 注意：该文件可以加载到Context Isolation页面的主世界中，
// 在浏览器环境中运行，而不是在节点环境中运行，均为本机。
// 模块必须从外部传递，所有包含的文件必须是纯JS。

import { WEB_VIEW_CONSTANTS } from '@electron/internal/renderer/web-view/web-view-constants';
import { WebViewImpl, WebViewImplHooks, setupMethods } from '@electron/internal/renderer/web-view/web-view-impl';
import type { SrcAttribute } from '@electron/internal/renderer/web-view/web-view-attributes';

const internals = new WeakMap<HTMLElement, WebViewImpl>();

// 返回在此上下文中定义的WebViewElement类。
const defineWebViewElement = (hooks: WebViewImplHooks) => {
  return class WebViewElement extends HTMLElement {
    static get observedAttributes () {
      return [
        WEB_VIEW_CONSTANTS.ATTRIBUTE_PARTITION,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_SRC,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_HTTPREFERRER,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_USERAGENT,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_NODEINTEGRATION,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_NODEINTEGRATIONINSUBFRAMES,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_PLUGINS,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_DISABLEWEBSECURITY,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_ALLOWPOPUPS,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_PRELOAD,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_BLINKFEATURES,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_DISABLEBLINKFEATURES,
        WEB_VIEW_CONSTANTS.ATTRIBUTE_WEBPREFERENCES
      ];
    }

    constructor () {
      super();
      internals.set(this, new WebViewImpl(this, hooks));
    }

    getWebContentsId () {
      const internal = internals.get(this);
      if (!internal || !internal.guestInstanceId) {
        throw new Error(WEB_VIEW_CONSTANTS.ERROR_MSG_NOT_ATTACHED);
      }
      return internal.guestInstanceId;
    }

    connectedCallback () {
      const internal = internals.get(this);
      if (!internal) {
        return;
      }
      if (!internal.elementAttached) {
        hooks.guestViewInternal.registerEvents(internal.viewInstanceId, {
          dispatchEvent: internal.dispatchEvent.bind(internal)
        });
        internal.elementAttached = true;
        (internal.attributes.get(WEB_VIEW_CONSTANTS.ATTRIBUTE_SRC) as SrcAttribute).parse();
      }
    }

    attributeChangedCallback (name: string, oldValue: any, newValue: any) {
      const internal = internals.get(this);
      if (internal) {
        internal.handleWebviewAttributeMutation(name, oldValue, newValue);
      }
    }

    disconnectedCallback () {
      const internal = internals.get(this);
      if (!internal) {
        return;
      }
      hooks.guestViewInternal.deregisterEvents(internal.viewInstanceId);
      if (internal.guestInstanceId) {
        hooks.guestViewInternal.detachGuest(internal.guestInstanceId);
      }
      internal.elementAttached = false;
      internal.reset();
    }
  };
};

// 注册&lt;webview&gt;自定义元素。
const registerWebViewElement = (hooks: WebViewImplHooks) => {
  // 我希望埃斯林特不是那么愚蠢，但它确实是。
  // Eslint-禁用-下一行。
  const WebViewElement = defineWebViewElement(hooks) as unknown as typeof ElectronInternal.WebViewElement

  setupMethods(WebViewElement, hooks);

  // CustomElements.Define必须在特殊范围内调用。
  hooks.allowGuestViewElementDefinition(window, () => {
    window.customElements.define('webview', WebViewElement);
    window.WebView = WebViewElement;

    // 删除回调，这样开发人员就不会调用它们并产生意外的。
    // 行为。
    delete WebViewElement.prototype.connectedCallback;
    delete WebViewElement.prototype.disconnectedCallback;
    delete WebViewElement.prototype.attributeChangedCallback;

    // 既然已经检索到|servedAttributes|，我们就可以将其隐藏起来。
    // 用户代码也是如此。
    // TypeScript担心我们正在删除一个只读属性。
    delete (WebViewElement as any).observedAttributes;
  });
};

// 准备注册&lt;webview&gt;元素。
export const setupWebView = (hooks: WebViewImplHooks) => {
  const useCapture = true;
  const listener = (event: Event) => {
    if (document.readyState === 'loading') {
      return;
    }

    registerWebViewElement(hooks);

    window.removeEventListener(event.type, listener, useCapture);
  };

  window.addEventListener('readystatechange', listener, useCapture);
};
