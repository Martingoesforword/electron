import { expect } from 'chai';
import { BrowserWindow } from 'electron/main';
import { emittedOnce } from './events-helpers';

async function ensureWindowIsClosed (window: BrowserWindow | null) {
  if (window && !window.isDestroyed()) {
    if (window.webContents && !window.webContents.isDestroyed()) {
      // 如果窗口尚未销毁，并且它具有未销毁的WebContents，
      // 那么调用Destroy()不会立即销毁它，因为它可能已经销毁了。
      // &lt;webview&gt;需要先销毁的子项。那样的话，我们。
      // 等待发出完全关闭信号的“Closed”事件。
      // 窗户。
      const isClosed = emittedOnce(window, 'closed');
      window.destroy();
      await isClosed;
    } else {
      // 如果没有WebContents或者WebContents已被销毁，
      // 则“Closed”事件已经发出，因此没有什么可以。
      // 等一下。
      window.destroy();
    }
  }
}

export const closeWindow = async (
  window: BrowserWindow | null = null,
  { assertNotWindows } = { assertNotWindows: true }
) => {
  await ensureWindowIsClosed(window);

  if (assertNotWindows) {
    const windows = BrowserWindow.getAllWindows();
    try {
      expect(windows).to.have.lengthOf(0);
    } finally {
      for (const win of windows) {
        await ensureWindowIsClosed(win);
      }
    }
  }
};

export async function closeAllWindows () {
  for (const w of BrowserWindow.getAllWindows()) {
    await closeWindow(w, { assertNotWindows: false });
  }
}
