import { IpcMainImpl } from '@electron/internal/browser/ipc-main-impl';

export const ipcMainInternal = new IpcMainImpl() as ElectronInternal.IpcMainInternal;

// 当通道名称为“Error”时，不要抛出异常。
ipcMainInternal.on('error', () => {});
