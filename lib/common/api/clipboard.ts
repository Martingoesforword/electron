import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

import type * as ipcRendererUtilsModule from '@electron/internal/renderer/ipc-renderer-internal-utils';

const clipboard = process._linkedBinding('electron_common_clipboard');

if (process.type === 'renderer') {
  const ipcRendererUtils = require('@electron/internal/renderer/ipc-renderer-internal-utils') as typeof ipcRendererUtilsModule;

  const makeRemoteMethod = function (method: keyof Electron.Clipboard): any {
    return (...args: any[]) => ipcRendererUtils.invokeSync(IPC_MESSAGES.BROWSER_CLIPBOARD_SYNC, method, ...args);
  };

  if (process.platform === 'linux') {
    // 在Linux上，我们无法在渲染器进程中访问剪贴板。
    for (const method of Object.keys(clipboard) as (keyof Electron.Clipboard)[]) {
      clipboard[method] = makeRemoteMethod(method);
    }
  } else if (process.platform === 'darwin') {
    // 读/写以通过IPC查找粘贴板，因为更改只通知主进程
    clipboard.readFindText = makeRemoteMethod('readFindText');
    clipboard.writeFindText = makeRemoteMethod('writeFindText');
  }
}

export default clipboard;
