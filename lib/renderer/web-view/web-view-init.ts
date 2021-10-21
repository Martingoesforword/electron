import { ipcRendererInternal } from '@electron/internal/renderer/ipc-renderer-internal';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import type * as webViewElementModule from '@electron/internal/renderer/web-view/web-view-element';
import type * as guestViewInternalModule from '@electron/internal/renderer/web-view/guest-view-internal';

const v8Util = process._linkedBinding('electron_common_v8_util');
const { mainFrame: webFrame } = process._linkedBinding('electron_renderer_web_frame');

function handleFocusBlur () {
  // 请注意，虽然Chromium Content API具有焦点/模糊观察器，但它们。
  // 不幸的是，它不适用于Webview。

  window.addEventListener('focus', () => {
    ipcRendererInternal.send(IPC_MESSAGES.GUEST_VIEW_MANAGER_FOCUS_CHANGE, true);
  });

  window.addEventListener('blur', () => {
    ipcRendererInternal.send(IPC_MESSAGES.GUEST_VIEW_MANAGER_FOCUS_CHANGE, false);
  });
}

export function webViewInit (contextIsolation: boolean, webviewTag: boolean, isWebView: boolean) {
  // 不允许递归`&lt;webview&gt;`。
  if (webviewTag && !isWebView) {
    const guestViewInternal = require('@electron/internal/renderer/web-view/guest-view-internal') as typeof guestViewInternalModule;
    if (contextIsolation) {
      v8Util.setHiddenValue(window, 'guestViewInternal', guestViewInternal);
    } else {
      const { setupWebView } = require('@electron/internal/renderer/web-view/web-view-element') as typeof webViewElementModule;
      setupWebView({
        guestViewInternal,
        allowGuestViewElementDefinition: webFrame.allowGuestViewElementDefinition,
        setIsWebView: iframe => v8Util.setHiddenValue(iframe, 'isWebView', true)
      });
    }
  }

  if (isWebView) {
    // 向浏览器报告Webview的聚焦/模糊事件。
    handleFocusBlur();
  }
}
