import type { WebViewImpl } from '@electron/internal/renderer/web-view/web-view-impl';
import { WEB_VIEW_CONSTANTS } from '@electron/internal/renderer/web-view/web-view-constants';

const resolveURL = function (url?: string | null) {
  return url ? new URL(url, location.href).href : '';
};

interface MutationHandler {
  handleMutation (_oldValue: any, _newValue: any): any;
}

// 属性对象。
// WebView属性的默认实现。
export class WebViewAttribute implements MutationHandler {
  public value: any;
  public ignoreMutation = false;

  constructor (public name: string, public webViewImpl: WebViewImpl) {
    this.name = name;
    this.value = (webViewImpl.webviewNode as Record<string, any>)[name] || '';
    this.webViewImpl = webViewImpl;
    this.defineProperty();
  }

  // 检索并返回属性的值。
  public getValue () {
    return this.webViewImpl.webviewNode.getAttribute(this.name) || this.value;
  }

  // 设置属性的值。
  public setValue (value: any) {
    this.webViewImpl.webviewNode.setAttribute(this.name, value || '');
  }

  // 更改属性的值，而不触发其突变处理程序。
  public setValueIgnoreMutation (value: any) {
    this.ignoreMutation = true;
    this.setValue(value);
    this.ignoreMutation = false;
  }

  // 将此属性定义为WebView节点上的属性。
  public defineProperty () {
    return Object.defineProperty(this.webViewImpl.webviewNode, this.name, {
      get: () => {
        return this.getValue();
      },
      set: (value) => {
        return this.setValue(value);
      },
      enumerable: true
    });
  }

  // 在属性值更改时调用。
  public handleMutation: MutationHandler['handleMutation'] = () => undefined
}

// 被视为布尔值的属性。
class BooleanAttribute extends WebViewAttribute {
  getValue () {
    return this.webViewImpl.webviewNode.hasAttribute(this.name);
  }

  setValue (value: boolean) {
    if (value) {
      this.webViewImpl.webviewNode.setAttribute(this.name, '');
    } else {
      this.webViewImpl.webviewNode.removeAttribute(this.name);
    }
  }
}

// 表示存储分区状态的属性。
export class PartitionAttribute extends WebViewAttribute {
  public validPartitionId = true

  constructor (public webViewImpl: WebViewImpl) {
    super(WEB_VIEW_CONSTANTS.ATTRIBUTE_PARTITION, webViewImpl);
  }

  public handleMutation = (oldValue: any, newValue: any) => {
    newValue = newValue || '';

    // 如果Web视图已导航，则无法更改分区。
    if (!this.webViewImpl.beforeFirstNavigation) {
      console.error(WEB_VIEW_CONSTANTS.ERROR_MSG_ALREADY_NAVIGATED);
      this.setValueIgnoreMutation(oldValue);
      return;
    }
    if (newValue === 'persist:') {
      this.validPartitionId = false;
      console.error(WEB_VIEW_CONSTANTS.ERROR_MSG_INVALID_PARTITION_ATTRIBUTE);
    }
  }
}

// 属性，该属性处理Web视图的位置和导航。
export class SrcAttribute extends WebViewAttribute {
  public observer!: MutationObserver;

  constructor (public webViewImpl: WebViewImpl) {
    super(WEB_VIEW_CONSTANTS.ATTRIBUTE_SRC, webViewImpl);
    this.setupMutationObserver();
  }

  public getValue () {
    if (this.webViewImpl.webviewNode.hasAttribute(this.name)) {
      return resolveURL(this.webViewImpl.webviewNode.getAttribute(this.name));
    } else {
      return this.value;
    }
  }

  public setValueIgnoreMutation (value: any) {
    super.setValueIgnoreMutation(value);

    // 需要TakeRecords()来清除排队的src突变。没有它，它就会。
    // 这一更改可能会被src的。
    // 变异观察者|观察者|，然后得到处理，即使我们没有。
    // 想要处理这种变异。
    this.observer.takeRecords();
  }

  public handleMutation = (oldValue: any, newValue: any) => {
    // 导航之后，我们不允许清除src属性。
    // 一旦进入导航状态，它就无法返回到。
    // 占位符状态。
    if (!newValue && oldValue) {
      // SRC属性更改通常会启动导航。我们压制。
      // 下一个src属性处理程序调用以避免重新加载页面。
      // 在每个来宾发起的导航上。
      this.setValueIgnoreMutation(oldValue);
      return;
    }
    this.parse();
  }

  // 此变异观测器的目的是捕捉对src的赋值。
  // 属性，而不对其值进行任何更改。这在这种情况下很有用。
  // Webview来宾已崩溃并导航到相同地址的位置。
  // 产生了一个新的过程。
  public setupMutationObserver () {
    this.observer = new MutationObserver((mutations) => {
      for (const mutation of mutations) {
        const { oldValue } = mutation;
        const newValue = this.getValue();
        if (oldValue !== newValue) {
          return;
        }
        this.handleMutation(oldValue, newValue);
      }
    });

    const params = {
      attributes: true,
      attributeOldValue: true,
      attributeFilter: [this.name]
    };

    this.observer.observe(this.webViewImpl.webviewNode, params);
  }

  public parse () {
    if (!this.webViewImpl.elementAttached || !(this.webViewImpl.attributes.get(WEB_VIEW_CONSTANTS.ATTRIBUTE_PARTITION) as PartitionAttribute).validPartitionId || !this.getValue()) {
      return;
    }
    if (this.webViewImpl.guestInstanceId == null) {
      if (this.webViewImpl.beforeFirstNavigation) {
        this.webViewImpl.beforeFirstNavigation = false;
        this.webViewImpl.createGuest();
      }
      return;
    }

    // 导航至|this.src|。
    const opts: Record<string, string> = {};

    const httpreferrer = this.webViewImpl.attributes.get(WEB_VIEW_CONSTANTS.ATTRIBUTE_HTTPREFERRER)!.getValue();
    if (httpreferrer) {
      opts.httpReferrer = httpreferrer;
    }

    const useragent = this.webViewImpl.attributes.get(WEB_VIEW_CONSTANTS.ATTRIBUTE_USERAGENT)!.getValue();
    if (useragent) {
      opts.userAgent = useragent;
    }

    (this.webViewImpl.webviewNode as Electron.WebviewTag).loadURL(this.getValue(), opts);
  }
}

// 属性指定HTTP Referrer。
class HttpReferrerAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_CONSTANTS.ATTRIBUTE_HTTPREFERRER, webViewImpl);
  }
}

// 属性指定用户代理。
class UserAgentAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_CONSTANTS.ATTRIBUTE_USERAGENT, webViewImpl);
  }
}

// 设置预加载脚本的属性。
class PreloadAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_CONSTANTS.ATTRIBUTE_PRELOAD, webViewImpl);
  }

  public getValue () {
    if (!this.webViewImpl.webviewNode.hasAttribute(this.name)) {
      return this.value;
    }

    let preload = resolveURL(this.webViewImpl.webviewNode.getAttribute(this.name));
    const protocol = preload.substr(0, 5);

    if (protocol !== 'file:') {
      console.error(WEB_VIEW_CONSTANTS.ERROR_MSG_INVALID_PRELOAD_ATTRIBUTE);
      preload = '';
    }

    return preload;
  }
}

// 属性，该属性指定要启用的闪烁功能。
class BlinkFeaturesAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_CONSTANTS.ATTRIBUTE_BLINKFEATURES, webViewImpl);
  }
}

// 属性，该属性指定要禁用的闪烁功能。
class DisableBlinkFeaturesAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_CONSTANTS.ATTRIBUTE_DISABLEBLINKFEATURES, webViewImpl);
  }
}

// 属性，该属性指定要启用的Web首选项。
class WebPreferencesAttribute extends WebViewAttribute {
  constructor (webViewImpl: WebViewImpl) {
    super(WEB_VIEW_CONSTANTS.ATTRIBUTE_WEBPREFERENCES, webViewImpl);
  }
}

// 设置所有WebView属性。
export function setupWebViewAttributes (self: WebViewImpl) {
  return new Map<string, WebViewAttribute>([
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_PARTITION, new PartitionAttribute(self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_SRC, new SrcAttribute(self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_HTTPREFERRER, new HttpReferrerAttribute(self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_USERAGENT, new UserAgentAttribute(self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_NODEINTEGRATION, new BooleanAttribute(WEB_VIEW_CONSTANTS.ATTRIBUTE_NODEINTEGRATION, self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_NODEINTEGRATIONINSUBFRAMES, new BooleanAttribute(WEB_VIEW_CONSTANTS.ATTRIBUTE_NODEINTEGRATIONINSUBFRAMES, self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_PLUGINS, new BooleanAttribute(WEB_VIEW_CONSTANTS.ATTRIBUTE_PLUGINS, self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_DISABLEWEBSECURITY, new BooleanAttribute(WEB_VIEW_CONSTANTS.ATTRIBUTE_DISABLEWEBSECURITY, self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_ALLOWPOPUPS, new BooleanAttribute(WEB_VIEW_CONSTANTS.ATTRIBUTE_ALLOWPOPUPS, self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_PRELOAD, new PreloadAttribute(self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_BLINKFEATURES, new BlinkFeaturesAttribute(self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_DISABLEBLINKFEATURES, new DisableBlinkFeaturesAttribute(self)],
    [WEB_VIEW_CONSTANTS.ATTRIBUTE_WEBPREFERENCES, new WebPreferencesAttribute(self)]
  ]);
}
