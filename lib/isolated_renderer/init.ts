/* 全球隔离的Api。*/

import type * as webViewElementModule from '@electron/internal/renderer/web-view/web-view-element';

if (isolatedApi.guestViewInternal) {
  // 必须在主世界中设置WebView元素。
  const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element') as typeof webViewElementModule;
  setupWebView(isolatedApi);
}
