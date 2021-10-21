import { webFrame, WebFrame } from 'electron';
import * as ipcRendererUtils from '@electron/internal/renderer/ipc-renderer-internal-utils';
import { IPC_MESSAGES } from '@electron/internal/common/ipc-messages';

// 扩展功能的WebFrame的所有键。
type WebFrameMethod = {
  [K in keyof WebFrame]:
    WebFrame[K] extends Function ? K : never
}

export const webFrameInit = () => {
  // 调用webFrame方法。
  ipcRendererUtils.handle(IPC_MESSAGES.RENDERER_WEB_FRAME_METHOD, (
    event, method: keyof WebFrameMethod, ...args: any[]
  ) => {
    // TypeScript编译器无法处理。
    // 请在这里签名，然后干脆放弃。不正确的调用。
    // 将被“keyof WebFrameMethod”捕获。
    return (webFrame[method] as any)(...args);
  });
};
